#!/bin/sh

REPO_URL=https://github.com/nlohmann/json.git
REPO_NAME=json
VERSION=v3.11.2
TARGET=single_include/nlohmann

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
