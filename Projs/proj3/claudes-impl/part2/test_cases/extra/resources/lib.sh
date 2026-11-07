# Sourced by the extra-test scripts, which run from the part's directory.
# Each script is called as: script.sh "<server command>" <test number>, and
# binds port PORT_BASE + <test number>. (The input lines avoid a literal '$':
# testius waits for output ending in '$' as its prompt, and a pty echo split
# just after one makes it type the next line too early.)
if [ -z "$PORT_BASE" ]; then
    echo "PORT_BASE is not set: test n binds port PORT_BASE + n"
    exit 1
fi
SERVER=$1
PORT=$((PORT_BASE + $2))
RES=test_cases/extra/resources
CLIENT="python3 $RES/http_client.py"
WORK=test_results/extra/$(basename "$0" .sh)
rm -rf "$WORK"
mkdir -p "$WORK"

# start_server DIR [launcher...]: start $SERVER in the background on $PORT, its
# output going to $WORK/server.out, and wait until it listens
start_server() {
    local dir=$1
    shift
    "$@" $SERVER "$dir" "$PORT" > "$WORK/server.out" 2>&1 &
    SERVER_PID=$!
    if ! $CLIENT wait-listen "$PORT"; then
        kill -KILL "$SERVER_PID" 2> /dev/null
        wait "$SERVER_PID" 2> /dev/null
        echo "server output:"
        cat "$WORK/server.out"
        exit 1
    fi
}

# finish_server [SECONDS]: wait for a server that was already sent SIGINT
finish_server() {
    python3 "$RES/wait_exit.py" "$SERVER_PID" "${1:-5}"
    wait "$SERVER_PID"
    echo "server exit status: $?"
}

# stop_server [SECONDS]: SIGINT, then report how the server ended
stop_server() {
    kill -INT "$SERVER_PID"
    finish_server "$@"
}

# FILES: every file in server_files, in a fixed order
FILES="quote.txt headers.html index.html courses.txt mt2_practice.pdf gatsby.txt africa.jpg ocelot.jpg hard_drive.png Lec01.pdf rain.mp3"

# curl_all PORT OUTDIR N: N concurrent curl downloads cycling through FILES,
# then report how many arrived intact. Each download streams into cmp rather
# than to disk (40 of them are about 40 MB); curl's errors go to OUTDIR.
curl_all() {
    local port=$1 out=$2 n=$3 i=0 files=($FILES) pids=()
    mkdir -p "$out"
    for ((i = 0; i < n; i++)); do
        local f=${files[i % ${#files[@]}]}
        {
            curl -s -S --max-time 20 "http://localhost:$port/$f" 2> "$out/$i-$f.err" |
                cmp -s - "server_files/$f" && touch "$out/$i-$f.ok"
        } &
        pids+=($!)
    done
    wait "${pids[@]}"
    local good=0
    for ((i = 0; i < n; i++)); do
        local f=${files[i % ${#files[@]}]}
        if [ -e "$out/$i-$f.ok" ]; then
            good=$((good + 1))
        else
            echo "download $i ($f) differs: $(head -c 200 "$out/$i-$f.err")"
        fi
    done
    echo "$n concurrent downloads, $good identical to the originals"
}

# server_output: for runs where a correct server has nothing to say
server_output() {
    if [ -s "$WORK/server.out" ]; then
        echo "server printed:"
        cat "$WORK/server.out"
    else
        echo "server printed nothing"
    fi
}
