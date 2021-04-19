
if [[ $1 == "debug" ]]; then
    shift
    CK_FORK=no gdb -q $*
    exit 0
fi

watch.sh 'src/* test/* Makefile' "\
 make all test && make coverage-html; \
 make --always-make --dry-run \
 | grep -wE 'clang' \
 | grep -w '\-c' \
 | jq -nR '[inputs|{directory:\".\", command:., file: match(\" [^ ]+$\").string[1:]}]' \
 > compile_commands.json"


