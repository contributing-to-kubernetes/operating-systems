# Booting Up a Computer

In order to understand operating systems (OSs), we need to first understand a
bit about computer architecture - all abstractions go out the door (or most of
them, at least).

So where should we start?
With a normal program, we could install a compiler and get to writing.
But if we want to do OS development, then we don't have any compiler.
What we do have is a machine and whatever data comes with it.
The one convenience tool we have is some sort of assembly language that we can
use to tell our machine to do things.

Our first task, boot a computer!
If we want to build an OS/kernel, then we will first need to get to an
environment in which we can do some basic data control: load some data into
memory, read data from keyboard, print it to screen, tell the machine to
perform some arbitrary task we want.

For the most part, the only help we have right now will be from the
[Basic Input/Output System (BIOS)](https://en.wikipedia.org/wiki/BIOS).
This firmware comes pre-installed.
As the name entails, this "system" (a collection of instructions) helps us
manage our machines devices (e.g., hard disk, keyboard, screen).
Using these tools we will eventually "execute" our operating system.
But before we do that let's get better acquainted with the BIOS, assembly, and
a let's talk a bit about hard disks/drives.

To begin making sense of all this we will need a development environment that
we will setup right now!

---

## Configuring Our Development Environment

Yah know, we could write some code, make a virtual machine image and run it in
virtualbox.
Hell, we could put our code in a flash drive (a floppy disks :save: lol) and
boot a real machine with our code.
Though, a simpler and quicker solution is to use a
[CPU Emulator](https://en.wikipedia.org/wiki/Emulator).

We will mostly mess around with i386-like architectures and x86.
The number of bits for computer architectures is mstly associated with the size
of the general purpose
[processor registers](https://en.wikipedia.org/wiki/Processor_register)
the machines had.
Overall, if you understand the examples here then generalizing to other
architectures is "doable".

Some good options for CPU emulation that we can use are

* QEMU https://www.qemu.org/
* BOCHS http://bochs.sourceforge.net/getcurrent.html

Bochs is mostly for emulating i386 (IA-32) architectures.
QEMU, on the other hand, can emulate various machine architectures.
So we will stick with qemu for the most part.

You can find installationg instructions in its site
https://www.qemu.org/download/.
Although you may need to do some googling around.

For example, in the machine that were this is being written (Ubuntu 19.10), the
actual installation was done by
```
apt install qemu-system-x86
```

This resulted in the `qemu-system-i386` and `qemu-system-x86_64` executables
being installed.

---

## Testing Out Our QEMU

It is always fun to experiment with a program after installing it.
For our first experiment we will do something that will put us on the road to
be OS developers...

If we go back to our computer we got devices and some firmware (the BIOS).
At some point we will want to run an operating system here.
And the way to do that is by telling the BIOS to load our operating system into
memory and let it take control of the machine.
This will be the first "program" we will write and in professional circles it
is called a **boot loader**.

The trickiness comes about us having to manually tell the computer how to do
this.
There are no `os.Open(filename)` operation.
As a matter of fact, there is no such thing as files!
Files are something we will create later on after we have a simple kernel.

For our boot loader, we will make use of some conventions.
In the times before high speed internet, people came up with the convention
that the boot boot loader program would be discovered by the BIOS by reading
the machines hard drives until it finds a **magic number**.
A magic number in this case is a signature telling the bios that the current
chunk of hard drive is a boot loader and that it must be executed before
anything else happens (since it typically loads an OS).

What happens is that the BIOS will start reading all the data in allthe hard
drives that it has available.
Usually things will be organized in
[**sectors**](https://en.wikipedia.org/wiki/Disk_sector).
When i386 architectures were the thing, sectors were 512 bytes in size.
But as computers became more and more rpevalent in every day life, the sectors
became 4k in size.
But because 512 bytes sized sectors were so popular and changing things takes a
lot of effort, something called 512e sectors was introduced.
These 512e sectors emulate 512 sectors on 4k sectors, which for us it just
means that we can continue working with the assumption that a secotr can be 512
bytes long.
For more details on all this sector business, please read
[Understanding Hard Disk Sector Size, Part 1: The Change](https://www.mirazon.com/understanding-hard-disk-sector-size-part-1-the-change/).

Back to our boot loader.
In order for the BIOS to load it into memory and let it run, we need to put a
magic number at the last 2 bytes of a sector.
This is because the machine reads 1 sector at a time.
As per the magic number being in the last 2 bytes of sector, well that is just
the convention.

Ourmagic number will be `0xaa55` (a hexadecimal number).

### Assembly Time

Let's get to making our boot loader.

We will start with some Intel assembly syntax (the other kind in i386/x86
architecture land is AT&T syntax).
For this we will need the netwide assembler (like a compiler but for assembly),
[**NASM**](https://www.nasm.us/).

You may need to do some googling on how to install nasm.
For ubuntu/debian, an
```
apt-get install -y nasm
```

will do the trick.

Now to our boot loader.
Tis one will be super simple, we want to add the magic number in the right
place so that the BIOS knows to execute it and for now we will simply have the
boot loader looping eternally (we don't really have any OS to load yet).

In assembly (intel syntax), the way we make a loop is with the use of the `JMP`
instruction, see https://en.wikipedia.org/wiki/JMP_(x86_instruction).
```assembly
loop:
  JMP loop
```

We simply create a label called `loop` to point to some place in memory and
then we tell the machine to JuMP back to it.

Now the magic number!
We want `0xaa55` at the end of the 512 bytes long sector.
So let's do a bit of math.

A hexadecimal number goes from 0 to 15 so we need 4 bits (because 2^4 == 16)
for each digit.
Our magic number, `0xaa55` has 4 digits, which means it needs 4 bits x 4 == 16
bits or 2 bytes of space.
So the other 510 bytes of sector can have our infinite loop plus whatever other
data (it doesn't matter what else we put in there since the loop is infinite).

After the infinite loop, we will fill everything up to 510 bytes with 0s.
To do this we will present to you some of assembly:

* `dw` - stands for Define Word. A word in a 32 bit system is 2 bytes (enough
  for a magic number).
* `db` - stands for Define Byte. Is a pseudo-instruction that declares 1 byte of
  data.
* `times` - is a utility offered by nasm to mean that the following instruction
  must be assembled multiple times (`times number_of_times instruction`).
* `$` - is an expression that evaluates to the assembly position at the beginning
  of the line containing the expression.
* `$$` - is an expression that evaluates to the beginning of the current
  section.
  * We can tell how far into the section we are by using `$-$$` (**section**
    here has a special meaning, please read
    https://www.tutorialspoint.com/assembly_programming/assembly_basic_syntax.htm ).

With that, our boot sector (our boot loader) will look as follows

```assembly
; Infinite loop.
loop:
    jmp loop

; Fill with 0s everything from our current location ($-$$) up to the 510 byte.
times 510-($-$$) db 0

; Magic number.
dw 0xaa55
```

This cmes already included in [boot_loader.asm](./boot_loader.asm).

The next step is to assemble our program
```
nasm boot_loader.asm -f bin -o boot_loader.bin
```

and then we boot our machine with our boot loader :smile:
```
qemu-system-i386 boot_loader.bin
```

If that works, then you will see a message with "Booting from hard disk".
Nothing else will happen since we are not really doing anything.
Though, you should try starting up your machine without a boot sector, try
`qemu-system-i386` by itself (you will see how the BIOS works and you will see
that your boot loader is being loaded).
