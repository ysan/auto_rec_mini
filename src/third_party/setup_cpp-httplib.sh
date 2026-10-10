#!/bin/bash

REPO_URL=https://github.com/yhirose/cpp-httplib.git
REPO_NAME=cpp-httplib
VERSION=v0.11.2
TARGET=httplib.h

set -e

rm -rf "./${TARGET}"
rm -rf "/tmp/${REPO_NAME}"

git clone ${REPO_URL} -b ${VERSION} --single-branch --depth 1 "/tmp/${REPO_NAME}"
(
	cd "/tmp/${REPO_NAME}"
	git checkout -b ${VERSION} "refs/tags/${VERSION}"
	git branch
)
cp -pr "/tmp/${REPO_NAME}/${TARGET}" .

rm -rf "/tmp/${REPO_NAME}"
