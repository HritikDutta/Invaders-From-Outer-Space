@echo off

rem Build Libraries

pushd dependencies

pushd glad
call build
popd

pushd miniz
call build
popd

pushd stb
call build
popd

popd