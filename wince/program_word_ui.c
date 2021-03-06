/*
 * Copyright (c) 2012 Alexey Illarionov <littlesavage@rambler.ru>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "stdafx.h"
#include "sirfmemdump.h"

static unsigned DEFAULT_ADDR = 0x07fffe;
static unsigned DEFAULT_WORD = 0x0123;

static int init_dialog_controls(HWND dialog, struct serial_session_t *s);
static int request_program_word(HWND dialog, struct serial_session_t *s);

INT_PTR CALLBACK program_word_callback(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam)
{
	BOOL msg_handled = FALSE;
	BOOL close_dialog = FALSE;
	struct serial_session_t **current_session;
	SHINITDLGINFO shidi;

    switch (message)
    {
		case WM_INITDIALOG:
			shidi.dwMask = SHIDIM_FLAGS;
            shidi.dwFlags = SHIDIF_DONEBUTTON
				| SHIDIF_SIPDOWN
				| SHIDIF_SIZEDLGFULLSCREEN
				| SHIDIF_EMPTYMENU;
            shidi.hDlg = dialog;
            SHInitDialog(&shidi);

			/* Save current_session as DWL_USER */
			current_session = (struct serial_session_t **)lParam;
			assert(current_session);
			SetWindowLong(dialog, DWL_USER, (LONG)lParam);

			init_dialog_controls(dialog, *current_session);

			msg_handled = TRUE;
			break;

        case WM_COMMAND:
			current_session = (struct serial_session_t **)GetWindowLong(dialog, DWL_USER);
			assert(current_session);
			switch (LOWORD(wParam))
			{				
				case IDOK:
					if (request_program_word(dialog, *current_session) >= 0)
						close_dialog = TRUE;
					msg_handled = TRUE;
					break;
				case IDCANCEL:
					msg_handled = close_dialog = TRUE;
					break;
			}

            break;

        case WM_CLOSE:
			msg_handled = close_dialog = TRUE;
			break;
    }

	if (close_dialog) {
		EndDialog(dialog, LOWORD(wParam));
	}

    return (INT_PTR)msg_handled;
}

static int init_dialog_controls(HWND dialog, struct serial_session_t *s)
{
	TCHAR tmp[80];

	_sntprintf(tmp, sizeof(tmp)/sizeof(tmp[0]),
		TEXT("0x%x"), DEFAULT_ADDR);
	SetDlgItemText(dialog, IDC_ADDR, tmp);

   _sntprintf(tmp, sizeof(tmp)/sizeof(tmp[0]),
		TEXT("0x%x"), DEFAULT_WORD);
	SetDlgItemText(dialog, IDC_WORD, tmp);

	return 0;
}


static int request_program_word(HWND dialog, struct serial_session_t *s)
{
	int res;
	unsigned long addr, word;
	const TCHAR *err_msg;
	TCHAR *endptr;
	TCHAR tmp[80];

	res = 0;
	err_msg = NULL;

	if (!s) {
		res = -1, err_msg = TEXT("Not connected");
		goto request_dump_end;
	}

	/* Address */
	if (GetDlgItemText(dialog, IDC_ADDR, tmp, sizeof(tmp)/sizeof(tmp[0])) < 1) {
		res = -1, err_msg = TEXT("Wrong flash address");
		goto request_dump_end;
	}

	addr = _tcstoul(tmp, &endptr, 0);
	assert(endptr != NULL);
	if ((*endptr != 0)
		|| (addr > 0xffffffff)
		) {
		res = -1, err_msg = TEXT("Address is not a number");
		goto request_dump_end;
	}

	/* Word */
	if (GetDlgItemText(dialog, IDC_WORD, tmp, sizeof(tmp)/sizeof(tmp[0])) < 1)
	{
		res = -1, err_msg = TEXT("Wrong word/data");
		goto request_dump_end;
	}

	word = _tcstoul(tmp, &endptr, 0);
	assert(endptr != NULL);
	if ((*endptr != 0)
		|| (word > 0xffff)
		) {
		res = -1, err_msg = TEXT("Wrong word/data");
		goto request_dump_end;
	}

	DEFAULT_ADDR = addr;
	DEFAULT_WORD = word;

	res = serial_session_req_program_word(s, addr, word);
	if (res == -WAIT_TIMEOUT)
		err_msg = TEXT("Busy");
	else if (res < 0) {
		tmp[0]=0;
		serial_session_error(s, tmp, sizeof(tmp)/sizeof(tmp[0]));
		err_msg = tmp;
	}

request_dump_end:
	if (err_msg)
		MessageBox(dialog, err_msg, NULL, MB_OK | MB_ICONERROR);

	return res;
}
