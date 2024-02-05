dwmstatus
=========

A better version of dwmblocks (hopefully...)

## Motivation

``dwmblocks`` uses a retarded way to update scripts, requiring commands to send 
signals to force it to re-run the script. ``dwmstatus``, on other hand, let 
other programs to send their data at "their pace", dwmstatus only waits for 
data and displays accordingly. Specifically, ``dwmstatus`` waits for a newline 
from each program and displays the current line in its position. If one program 
updates but the other program doesn't, ``dwmstatus`` will only update the block.

This C program is so fucking stupid that i don't know why no one actually has 
thought of this before. Fuck...

## Usage
	$ dwmstatus
	usage: dwmstatus -c command [-c command2 ]...

Every -c argument is a shell script string. The order is reversed. 

## Example
	$ dwmstatus -c 'while true; do date ; sleep 1; done' -c './dwmstatus-pulse'
	100 | dom 04 fev 2024 21:36:05 -03
	100 | dom 04 fev 2024 21:36:06 -03
	100 | dom 04 fev 2024 21:36:07 -03
	100 | dom 04 fev 2024 21:36:08 -03
	100 | dom 04 fev 2024 21:36:09 -03
	99 | dom 04 fev 2024 21:36:09 -03
	98 | dom 04 fev 2024 21:36:09 -03
	98 | dom 04 fev 2024 21:36:09 -03
	97 | dom 04 fev 2024 21:36:09 -03
	97 | dom 04 fev 2024 21:36:09 -03

## License

This program is licensed with WTFPL. 

## Contribution

You can send your patches to my e-mail, which is shown in my signed commits, 
or you can send a pull request. You probably don't want to make a fork of this 
program to the server where this is originally hosted (git.lain.church) but 
whatever, do whatever you want, lolololololololol...
