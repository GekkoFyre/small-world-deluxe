@ECHO OFF

cd src/contrib
git submodule update --init --recursive

cd leveldb/third_party
git submodule update --init --recursive
cd ./../..

cd sentry-native\external
git submodule update --init --recursive
cd ./../../../..

ECHO Process has completed its assigned tasks, but please be sure to check if there are any errors/faults within the output.

PAUSE