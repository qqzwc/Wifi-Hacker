# WifiHacker
A simple program to hack WiFi nets.

Abilities:
==========
Brute force hacking.

Passwords list hacking.

# Usage:
## With Brute Force

**"Wifi Hacker.exe" /bf pass_length pass_characters**

pass_length->the length of the passwords.

pass_characters->the characters that can be in the passwords.

*Note: the brute force mode is slow and not recommended. Use it only if you know that the WiFi password is strong.
You can use PassGen instead to create random passwords lists.*

## With Passwords List

**"Wifi Hacker.exe" /pl pass_lst_path**

pass_lst_path->the *full* path to the passwords list.

After calling the program with the arguments, you can choose interface and network. Once the password is found, the program will stop and print the password.

## IMPORTANT
You might want to use a high-speed WiFi card. The connection process can take a long time!

# Compiling the program
The program was written is VS 2019, so you should use it to compile the code.

Of course, you don't have to use VS, you can compile it in every Windows compiler.

## IMPORTANT
If you use VS to compile the code, you should know that when compiling the code in 'Release' mode, the code freezes while waiting for the connection results. You can compile in 'Debug' mode to fix this, or you can add some code in the 'while(!next)' loop.
