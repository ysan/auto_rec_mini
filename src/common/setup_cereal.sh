#!/bin/sh

rm -rf /tmp/cereal

#git clone https://github.com/USCiLab/cereal.git /tmp/cereal
git clone https://github.com/USCiLab/cereal.git -b v1.3.0 --single-branch --depth 1 /tmp/cereal
(
	cd /tmp/cereal
	git checkout -b v1.3.0 refs/tags/v1.3.0
	git branch
)
cp -pr /tmp/cereal/include/cereal .

rm -rf /tmp/cereal
