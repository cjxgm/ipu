# ipu - Image Processing Unit: A Procedural Image Synthesizer
Licensed under The MIT License.<br>
Copyright (C) 2013, eXerigumo Clanjor (哆啦比猫/兰威举).<br>
See `COPYING` for more details.<br>
<br>
Written in `GNU C 11`, with `Enlightenment Foundation Library`.<br>
**Note**: This project is in its alpha stage.

<hr>
With `ipu`, you can invent image textures from scratch.
![preview](https://f.cloud.github.com/assets/490327/160616/08e2f270-776f-11e2-9179-e048c49d2c17.gif)

## Requirements

* Up-to-date modern **Linux** operating system (ArchLinux is recommended).
* gnu tool cahin (I mean, **gcc** and **make**)
* **bash** (for running configure script)
* **pkg-config** (nearly every modern Linux has it)
* Enlightenment Foundation Library ( **EFL** ) with **elementary**
* **libcrude**, which is already included in the source code.

## Have a try!

First, clone this repo:

```bash
git clone git://github.com/cjxgm/ipu.git
```

Then, compile it and run

```bash
cd ipu
./configure
make
make test
```

For it's an alpha project, "install" function is not provided. If you want
to, you can do it in this way:

```bash
sudo cp build/ipu /usr/bin
```

