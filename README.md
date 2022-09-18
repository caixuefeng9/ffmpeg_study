# Study
Summary

#在wsl2搭建ffmpeg开发环境
搭建mingw-w64编译环境，脚本网址：https://github.com/Zeranoe/mingw-w64-build
./mingw-w64-build x86_64

/*
 *如果运行脚本报如下错误：
 *missing required executable(s): flex bison makeinfo m4
 *说明flex bison等命令不支持，需要安装相应的工具
 */
//可以按照如下的方式安装相应的工具
caixuefeng@xuefeng:/mnt/d/Study/ffmpeg/mingw-w64-build-master$ flex

Command 'flex' not found, but can be installed with:

sudo apt install flex
sudo apt install flex-old

caixuefeng@xuefeng:/mnt/d/Study/ffmpeg/mingw-w64-build-master$ sudo apt install flex

sudo apt install mingw-w64-tools 安装mingw-w64-pkg-config

x86_64-w64-mingw32-gcc -g -o pcm_player pcm_player.c `x86_64-w64-mingw32-pkg-config --cflags --libs libsdl2 libsdl2`