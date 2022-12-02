#!/bin/sh -x

set -e

echo "install python package pexpect and command meld"

# package pexpect niveau utilisateur
pip3 install pexpect --user
pip3 install termcolor --user

# package pexpect niveau utilisateur
sudo yum install meld

# add execution rights to .py test script
chmod a+x ./execute_tests.py

echo "==> OK"
