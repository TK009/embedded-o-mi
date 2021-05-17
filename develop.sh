
if [[ $1 == "debug" ]]; then
    shift
    CK_FORK=no gdb -q $*
    exit 0
fi

watch.sh 'src/* test/* platforms/** Makefile' "\
 make all -j -O && make test && make coverage-html && make esp32s2 && \
 make --always-make --dry-run \
 | grep -wE 'clang' \
 | grep -w '\-c' \
 | jq -nR '[inputs|{directory:\".\", command:., file: match(\" [^ ]+$\").string[1:]}]' \
 > compile_commands.json"


