@echo off

echo EXPORTING GAME

if not exist export md export

copy main.exe "export\Invaders from Outer Space!.exe"
copy assets\settings export\assets
copy misc\changelog.txt export\changelog.txt