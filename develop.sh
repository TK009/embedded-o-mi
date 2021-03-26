
if [[ $1 == "debug" ]]; then
    shift
    CK_FORK=no gdb -q $*
    exit 0
fi

watch.sh 'src/* test/* Makefile' 'make test && make coverage-html'

