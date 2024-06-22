**My First MCU Program**
=============
这是一个通过STC的增强型8051单片机驱动120*48液晶黑白屏的小项目，是作者的第一个C语言项目，也是第一个单片机项目。非常感谢我的父亲在我学习初期给予的指导和支持，单片机硬件、程序中ST7567的初始化代码和驱动代码最初都是爸爸提供的。
**************************************************************************
A project of using STC89C516RD+ to drive LCD ST12048A01.
--------------
It's the first time I learn to program MCU. And some basic function came from my dad (LCD init,write_command,write_data...,thank my Dad very much).
Until 2024.3.5,I have spent about 2 weeks on learning how to program with C and finally finsh the first part. I really learned a lot during programing.
***************************
**项目说明**
==================
>首先是作者的一点随笔，想到什么写什么的那种。

>这是一个于2024年初寒假时开始的单片机小项目。最初目的是为了学习单片机的基本用法和简单的显示屏驱动。
>但是开始学习显示控制之后越学越觉得有意思，看着分辨率只有120*48的液晶屏就感觉黑白屏特别适合用来做一些像贪吃蛇这样的老游戏。于是就在寒假的最后一周开始了贪吃蛇小游戏的软件编写。
>初期遇到的第一个重大问题是单片机内存不足。只有256字节的传统内部RAM和1KB的片内扩展，如果将全部像素都缓存下来一次性就会吃掉720Byte，剩下的内存用起来就比较捉襟见肘了，也不利于后续升级扩展。所以在较为详细地分析了显示方式后，决定以4\*4像素的方块作为蛇的一节，这样以来显示缓存就只需要23\*2=46Byte了，内存占用大大减小。之后就在省内存的路上一去不复返了。为了避免反复刷新显示与全局刷新显示的低帧率影响观感，贪吃蛇的显示刷新采用了仅刷新头和尾的方式，通过记录贪吃蛇的移动行为来从头部“寻迹”到尾部再把尾巴擦掉（就是运算量有所增加，后来做俄罗斯方块的时候也这种方法也不太适用）。记录行为的时候发现只有四种情况，为了省内存还专门写了套子程序来把每个行为的内存占用压缩到2位数据。基本上可以使说“能运算得到的值就不用存储”的思路贯穿项目始终。

>在7567.c文件中可以看到大片的注释内容，基本都是作者学习写代码过程中的一些笔记或者测试代码~~测试代码剩那么多其实有一部分原因是作者懒得删，虽然留个纪念也不错~~，其作用不仅仅是内容的记录，同时也是为未来留下的回忆，以后再看到现在写的东西，就能想起自己以前原来这么菜，然后兴许能推动自己继续努力学习~~也可能就是徒增笑料~~。
*****************************
以下是更新说明：

_2024.3.5_
**v1.0**

>这是这个项目最早期的一个完整版本，该版本完成了贪吃蛇小游戏，已经在测试中修好了所有发现的bug。
>程序现在可以完美地通过行为记录锁定到贪吃蛇的尾部以进行尾部的擦除。

_2024.6.20_
**v1.3.1更新说明**

>由于之前有2个版本没上传，所以公告把之前的内容也一起说了。
>此次更新主要有以下内容：
>
>1.鸽了2个月的俄罗斯方块终于可玩啦！（连着捉了3个晚上虫，终于把坐标处理基本弄完善了）
>
>2.添加了简单的多级用户交互界面，现在可以使用主菜单、游戏选择界面和只有一页的系统介绍，系统设置暂时不可用。
>
>3.微调了部分函数，把一些变量重新定义到xdata区了。
******************************

**未来计划**
--------------
_短期目标_
------------
>1.优化交互界面各按键的储存结构，改为使用链表存储。
>
>2.修改交互界面显示架构，统一为共同架构。
>
>3.制作文本显示系统，实现多页文字显示。
>
>4.现在所有的代码都写在一个.c文件中，后续应该尝试分成几个.c文件以分离不同的函数功能
>
>5.看心情再制作一些小游戏。

_长期目标_
-------------
>1.将小游戏使用的4\*4方格显示系统升级为自动运算获得显示模式，当前为手动预设。
>
>2.逐步升级单片机及开发板，并通过EEPROM实现存档功能。
>
>3.优化系统架构，提高可移植性，先适配各种分辨率的黑白屏，再适配彩屏单色显示，最后实现适配彩屏彩色显示。
>
>4.扩展更多的外设接口，增加温度传感器、气压计等外设。

******************************
短期目标应该会继续在当前项目上升级，但长期目标大概率会新开一个项目去做，毕竟软件和硬件都要大改。

*******************************
**Update Descriptions**
-----------
_2024.3.5_
**Uploaded v1.0**
>Now ,the program can perfectly process the moving system of the snake.
>I wrote a bit-type coordinate system,which can calculate the snake's coordinates and recorde the status.
>The system is realized through the following steps:
>
>1.System read the right number of target coordinates column where the snake head was located.
>
>2.A 3-bit probe to scan the target coordinates column and return the results.(Repeat 3 times to scan all three "area")
>
>3.Data comes from the probe decide which kind of picture to be displayed on the screen.
>
>4.System calculate the location of snake tail.
>
>5.The probe scan again and decide how to erase the tail.
>
>6.Erasing the expired tail.
>
>In the next few days, I am going to make the program be able to create food randomly and limit the range.

_2024.6.20_
**Updated the files to v1.3.1**
>In this version, user can now access two games by a simple interface.Actually,if you take the earlist upload as the first version,ther were 2 versions I forgot to upload.However,this version contains all the updates, so it's alright ro leave the old versions away.

Here are the main updates:
>1.A simple interface has been added to the program,user can use it to access two games and system description.(system settings are still not available for now.)
>
>2.The moving speed of snake in Gluttonous Snake have been increased.
>
>3.A new game Tetris have been added.Please give it a try.Reporting bugs is always welcome.
>
>4.Move some variables to auxiliary RAM to reduse On-chip Scratch-Pad RAM usage.
>
>5.Other minor functional adjustments.

**************
_PS:I am not quite good at English, thus I am unable to translate all the Chinese descriptions into English.Please read Chinese description if you wish to know more. I am willing to answer all your questions about the project. Thank you for your understanding._
