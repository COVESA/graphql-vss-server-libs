#!/bin/sh

# Copyright (C) 2021, Bayerische Motoren Werke Aktiengesellschaft (BMW AG),
#   Author: Alexander Domin (Alexander.Domin@bmw.de)
# Copyright (C) 2021, ProFUSION Sistemas e Soluções LTDA,
#   Author: Gabriel Fernandes (g7fernandes@profusion.mobi)
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the
# Mozilla Public License, v. 2.0. If a copy of the MPL was
# not distributed with this file, You can obtain one at
# http://mozilla.org/MPL/2.0/.

HOOKS_DIR=$(dirname $PWD/$0)
GIT_ROOT_DIR=$(git rev-parse --show-toplevel)

install_pre_commit() {
    PRE_COMMIT_SCRIPT="${HOOKS_DIR}/pre-commit"
    PRE_COMMIT_HOOK="${GIT_ROOT_DIR}/.git/hooks/pre-commit"
    if [ -f "${PRE_COMMIT_SCRIPT}" ] && [ ! -f "${PRE_COMMIT_HOOK}" ]; then
        echo "installing ${PRE_COMMIT_SCRIPT} as ${PRE_COMMIT_HOOK}"
        ln -s "${PRE_COMMIT_SCRIPT}" "${PRE_COMMIT_HOOK}"
    else
        echo ".git/hooks/pre-commit already exists, not installing ${PRE_COMMIT_SCRIPT}."
    fi
}

install_pre_push() {
    PRE_PUSH_SCRIPT="${HOOKS_DIR}/pre-push"
    PRE_PUSH_HOOK="${GIT_ROOT_DIR}/.git/hooks/pre-push"
    if [ -f "${PRE_PUSH_SCRIPT}" ] && [ ! -f "${PRE_PUSH_HOOK}" ]; then
        echo "installing ${PRE_PUSH_SCRIPT} as ${PRE_PUSH_HOOK}"
        ln -s "${PRE_PUSH_SCRIPT}" "${PRE_PUSH_HOOK}"
    else
        echo ".git/hooks/pre-push already exists, not installing ${PRE_PUSH_SCRIPT}."
    fi
}

install_pre_commit &&
install_pre_push
