# WinKeylogger

Easy to use library, to integrate a keylogger in your application. Windows only.

The library is comprised of two parts. The source file `WinKeylogger.c` and the header file `WinKeylogger.h`.

To use it in your C/C++ program you have to do two things:

1) ```#include "WinKeylogger.h"```

2) Compile `WinKeylogger.c` together with your other source files. You could compile it in the form of a library and link with it, but it would be less painful to just add another source file on your project.

## Dependencies

WinAPI and C99 compatible compiler. No standard library functions or headers are used or included (explicitly at least). `WinKeylogger.h` includes `windef.h` .

## How To Use

The main API of WinKeylogger, is based on these functions:

~~~~C
BOOL StartWinKeylogger(void);
void RemoveWinKeylogger(void);
BOOL LogNextKeystroke(size_t *buff_len);
void StrCpyLoggedBuffer(wchar_t *szBuff);
~~~~

Before you do anything you want to initialize the keylogging thread and let it run in the background of your app.

~~~~c
#include <stdio.h>
#include "WinKeylogger.h"

int main(void)
{
	if (StartWinKeylogger() == FALSE) {
		printf("WinKeylogger failed!\n");
		return 1;
	}

	return 0;
}
~~~~

The function `StartWinKeylogger()` doesn't take any parameters and returns immediately. If everything went well, the keylogger will start running in the background. If it returned `FALSE` then something wasn't initialized properly.

If you want to clean-up on your program and eliminate all memory leaks, you could call the function `RemoveWinKeylogger()` after you're done using the keylogger in your application:

~~~~c
#include <stdio.h>
#include "WinKeylogger.h"

int main(void)
{
	if (StartWinKeylogger() == FALSE) {
		printf("WinKeylogger failed!\n");
		return 1;
	}

	RemoveWinKeylogger();
	return 0;
}
~~~~

Inside WinKeylogger there's a null-terminated buffer, of `wchar_t` type, that stores each character typed on the keyboard. The buffer has maximum length of `WINKEYLOGGER_BUFSIZ` and since it's null-terminated, the maximum amount  of characters it can hold, is `WINKEYLOGGER_BUFSIZ - 1`. This macro is defined in `WinKeylogger.h` and you can change it into whatever value you want.

However, WinKeylogger doesn't read characters inside the buffer on its own. To do that you'll have to call explicitly the *blocking* function `LogNextKeystroke()`. This function will block your program when you call it, until an acceptable keystroke is pressed (examples of unacceptable keystrokes are the F1-F12 keys, or the Home/End keys). When that keystroke is pressed, the corresponding character will be written in the buffer and `LogNextKeystroke()` will return. An example of its usage:

```c
#include <stdio.h>
#include "WinKeylogger.h"

int main(void)
{
	if (StartWinKeylogger() == FALSE) {
		printf("WinKeylogger failed!\n");
		return 1;
	}

	LogNextKeystroke(NULL);

	RemoveWinKeylogger();
	return 0;
}
```

`LogNextKeystroke()` takes one optional argument, a pointer to a `size_t` variable that stores the current size of the buffer, each time it's called. So for example this:

~~~~c
#include <stdio.h>
#include "WinKeylogger.h"

int main(void)
{
	if (StartWinKeylogger() == FALSE) {
		printf("WinKeylogger failed!\n");
		return 1;
	}

	size_t b_size = 0;
	LogNextKeystroke(&b_size);
	printf("b_size = %zu\n", b_size);

	RemoveWinKeylogger();
	return 0;
}
~~~~

Will print 1, if the call to `LogNextKeystroke()` was successful and 0 if it was unsuccessful. `LogNextKeystroke()` returns either `TRUE` or `FALSE`. It will return `FALSE` if the WinKeylogger thread stopped running for some reason (if you called `RemoveWinKeylogger()` from another thread that isn't blocked for example) and the `size_t` parameter that was passed will be 0.

It will also return `FALSE` if the user presses the ESCAPE key. This is just for debugging reasons. You can easily remove that part of the code.

To copy the internal `wchar_t` buffer into your own buffer, you could declare a `wchar_t` array with `WINKEYLOGGER_BUFSIZ` length and use the `StrCpyLoggedBuffer()` function:

~~~~c
#include <stdio.h>
#include "WinKeylogger.h"

int main(void)
{
	if (StartWinKeylogger() == FALSE) {
		printf("WinKeylogger failed!\n");
		return 1;
	}

	size_t b_size = 0;
	wchar_t my_buffer[WINKEYLOGGER_BUFSIZ];

	LogNextKeystroke(&b_size);
	LogNextKeystroke(&b_size);
	LogNextKeystroke(&b_size);

	StrCpyLoggedBuffer(my_buffer);

	if (b_size)
		wprintf(L"my_buffer = %s\n", my_buffer);

	RemoveWinKeylogger();
	return 0;
}
~~~~

What happens when the internal buffer reaches the `WINKEYLOGGER_BUFSIZ - 1` limit? The next keystrokes that will be read, will be written to the buffer starting from the beginning. Meaning that if the internal buffer reached its limit, it will start overwriting the old keystroke data, and that you should store it before you lose important keylogs.

This program stores and prints all printable input that the user enters, until the ESCAPE key is pressed:

~~~~c
#include <stdio.h>
#include "WinKeylogger.h"

int main(void)
{
	if (StartWinKeylogger() == FALSE) {
		printf("WinKeylogger failed!\n");
		return 1;
	}

	size_t b_size = 0;
	wchar_t my_buffer[WINKEYLOGGER_BUFSIZ];

	while (LogNextKeystroke(&b_size))
		if (b_size >= WINKEYLOGGER_BUFSIZ - 1) {
			StrCpyLoggedBuffer(my_buffer);
        	wprintf(L"Keystrokes recorded so far = \"%s\"\n", my_buffer);
		}

	if (b_size) {
		StrCpyLoggedBuffer(my_buffer);
		wprintf(L"Keystrokes recorded so far = \"%s\"\n", my_buffer);
	}

	RemoveWinKeylogger();
	return 0;
}
~~~~



## Caveats

WinKeylogger accepts as valid input the keys from 0 to 9, the keys from A to Z, the Spacebar key, Backspace (is converted to the `'_'` character and no characters are deleted from the buffer), Tab and Return key (is converted to a simple whitespace character). All other keys are regarded as non printable and `LogNextKeystroke()` won't return if any of them are pressed.

Obviously no localization or uppercase/lowercase is taken under consideration. WinKeylogger simply records the keystrokes and maps them to their uppercase latin alphabet equivalent (A-Z).

## LICENSE

See LICENSE.txt

WinKeylogger (C) 2017 George Koskeridis