[comment {-*- tcl -*- doctools = format-header.inc}]

[manpage_begin img-[vset FORMATNAME] n 1.4]
[moddesc Img]
[titledesc "Img, [vset FORMATFULLNAME] ([vset FORMATNAME])"]
[copyright {1995-2023 Jan Nijtmans <nijtmans@users.sourceforge.net>}]
[require Tk]
[require img::[vset FORMATNAME] [opt 1.4]]
[description]

[cmd "package require img::[vset FORMATNAME]"].

[para]

Like all packages of [package Img] it does [emph not] provide new
commands, but extends the existing [package Tk] command [cmd image] so
that it supports files containing raster images in the

[vset FORMATFULLNAME] ([vset FORMATNAME]). More specifically

[package img::[vset FORMATNAME]] extends [package Tk]'s [cmd photo]
image type.

[para]

The name of the new format handler is [const [vset FORMATNAME]].
