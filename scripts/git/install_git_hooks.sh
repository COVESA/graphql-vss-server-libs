#!/bin/sh

HOOKS_DIR=`dirname $PWD/$0`
GIT_ROOT_DIR=`git rev-parse --show-toplevel`

install_pre_commit() {
    PRE_COMMIT_SCRIPT="${HOOKS_DIR}/pre-commit"
    PRE_COMMIT_HOOK="${GIT_ROOT_DIR}/.git/hooks/pre-commit"
    if [ -f ${PRE_COMMIT_SCRIPT} ] && [ ! -f ${PRE_COMMIT_HOOK} ]; then
        echo "installing ${PRE_COMMIT_SCRIPT} as ${PRE_COMMIT_HOOK}"
        ln -s ${PRE_COMMIT_SCRIPT} ${PRE_COMMIT_HOOK}
    else
        echo ".git/hooks/pre-commit already exists, not installing ${PRE_COMMIT_SCRIPT}."
    fi
}

install_pre_commit
