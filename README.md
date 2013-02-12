# ipu - Image Processing Unit: An Image Synthesizer
Licensed under GPLv2.<br>
Copyright (C) eXerigumo Clanjor (哆啦比猫/兰威举), 2013.<br>
Written in `GNU C 11`, with `Enlightenment Foundation Library`.
**Note**: This project is in its alpha stage.

<hr>
With `ipu`, you can invent image textures from scratch.
![snap](https://f.cloud.github.com/assets/490327/148861/1f7df14e-7515-11e2-9175-abd53f810f30.png)

## Requirements

* Up-to-date modern **Linux** operating system (ArchLinux is recommended).
* gnu tool cahin (I mean, **gcc** and **make**)
* **bash** (for running configure script)
* **pkg-config** (nearly every modern Linux has it)
* Enlightenment Foundation Library (**EFL**) with **elementary**

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

