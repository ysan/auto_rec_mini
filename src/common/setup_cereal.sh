#!/bin/sh

REPO_URL=https://github.com/USCiLab/cereal.git
REPO_NAME=cereal
VERSION=v1.3.0
TARGET=include/cereal

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
