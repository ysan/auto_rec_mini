#!/bin/sh

REPO_URL=https://github.com/ysan/coseq.git
REPO_NAME=coseq
VERSION=v0.1.0

set -e

rm -rf "/tmp/${REPO_NAME}"

git clone ${REPO_URL} -b ${VERSION} --single-branch --depth 1 "/tmp/${REPO_NAME}"
(
	cd "/tmp/${REPO_NAME}"
	git checkout -b ${VERSION} "refs/tags/${VERSION}"
	git branch
)

# ソースのみ上書き (本リポジトリ側の Makefile は温存)
cp -p "/tmp/${REPO_NAME}/coseq/coseq.c"            coseq/coseq/coseq.c
cp -p "/tmp/${REPO_NAME}/coseq/coseq_if.h"         coseq/coseq/coseq_if.h
cp -p "/tmp/${REPO_NAME}/coseqpp/coseqpp.cpp"      coseq/coseqpp/coseqpp.cpp
cp -p "/tmp/${REPO_NAME}/coseqpp/coseqpp.h"        coseq/coseqpp/coseqpp.h
cp -p "/tmp/${REPO_NAME}/coseqpp/coseqpp_base.cpp" coseq/coseqpp/coseqpp_base.cpp
cp -p "/tmp/${REPO_NAME}/coseqpp/coseqpp_base.h"   coseq/coseqpp/coseqpp_base.h
cp -p "/tmp/${REPO_NAME}/coseqpp/coseqpp_if.h"     coseq/coseqpp/coseqpp_if.h

rm -rf "/tmp/${REPO_NAME}"
