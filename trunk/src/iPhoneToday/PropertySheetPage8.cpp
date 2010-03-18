//////////////////////////////////////////////////////////////////////////////
// PropertySheetPage8.cpp : Defines the property sheet page procedures.
//

#include "stdafx.h"
#include "iPhoneToday.h"
#include "OptionDialog.h"

/*************************************************************************/
/* General options dialog box procedure function                 */
/*************************************************************************/
LRESULT CALLBACK OptionDialog8(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			InitOptionsDialog(hDlg, 8);

			SendMessage(GetDlgItem(hDlg, IDC_CHECK_TRANS_ALPHABLEND),   BM_SETCHECK, configuracion->alphaBlend == 1? BST_CHECKED : BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(hDlg, IDC_CHECK_TRANS_ALPHABLEND2),  BM_SETCHECK, configuracion->alphaBlend == 2? BST_CHECKED : BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(hDlg, IDC_CHECK_TRANS_ALPHAONBLACK), BM_SETCHECK, configuracion->alphaOnBlack   ? BST_CHECKED : BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(hDlg, IDC_CHECK_TRANS_BMP),          BM_SETCHECK, configuracion->transparentBMP ? BST_CHECKED : BST_UNCHECKED, 0);
			SetDlgItemInt(hDlg, IDC_EDIT_TRANS_THRESHOLD, configuracion->alphaThreshold, TRUE);

			if (configuracion->alphaBlend) {
				EnableWindow(GetDlgItem(hDlg, IDC_CHECK_TRANS_ALPHAONBLACK), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_EDIT_TRANS_THRESHOLD),     FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_CHECK_TRANS_BMP),          FALSE);
				if (configuracion->alphaBlend == 1) {
					EnableWindow(GetDlgItem(hDlg, IDC_CHECK_TRANS_ALPHABLEND2), FALSE);
				}else if (configuracion->alphaBlend == 2) {
					EnableWindow(GetDlgItem(hDlg, IDC_CHECK_TRANS_ALPHABLEND), FALSE);
				}
			}
		}
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		BOOL checked;
		case IDC_CHECK_TRANS_ALPHABLEND:
			checked = SendMessage(GetDlgItem(hDlg, IDC_CHECK_TRANS_ALPHABLEND), BM_GETCHECK, 0, 0) == BST_CHECKED;
			if (checked && !nativelySupportsAlphaBlend()) {
				if (MessageBox(hDlg, L"Your device does not natively support AlphaBlend. Do you want to enable it? Scrolling will be slow!", L"Warning", MB_YESNO) == IDNO) {
					SendMessage(GetDlgItem(hDlg, IDC_CHECK_TRANS_ALPHABLEND), BM_SETCHECK, BST_UNCHECKED, 0);
					break;
				}
			}
			EnableWindow(GetDlgItem(hDlg, IDC_CHECK_TRANS_ALPHAONBLACK), !checked);
			EnableWindow(GetDlgItem(hDlg, IDC_EDIT_TRANS_THRESHOLD),     !checked);
			EnableWindow(GetDlgItem(hDlg, IDC_CHECK_TRANS_BMP),          !checked);
			EnableWindow(GetDlgItem(hDlg, IDC_CHECK_TRANS_ALPHABLEND2),  !checked);
			SendMessage(GetDlgItem(hDlg, IDC_CHECK_TRANS_ALPHABLEND2), BM_SETCHECK, BST_UNCHECKED, 0);
			break;
		case IDC_CHECK_TRANS_ALPHABLEND2:
			checked = SendMessage(GetDlgItem(hDlg, IDC_CHECK_TRANS_ALPHABLEND2), BM_GETCHECK, 0, 0) == BST_CHECKED;
			EnableWindow(GetDlgItem(hDlg, IDC_CHECK_TRANS_ALPHAONBLACK), !checked);
			EnableWindow(GetDlgItem(hDlg, IDC_EDIT_TRANS_THRESHOLD),     !checked);
			EnableWindow(GetDlgItem(hDlg, IDC_CHECK_TRANS_BMP),          !checked);
			EnableWindow(GetDlgItem(hDlg, IDC_CHECK_TRANS_ALPHABLEND),   !checked);
			SendMessage(GetDlgItem(hDlg, IDC_CHECK_TRANS_ALPHABLEND), BM_SETCHECK, BST_UNCHECKED, 0);
			break;
		}
		return 0;
	case WM_PAINT:
		PaintOptionsDialog(hDlg, 8);
		return 0;
	case WM_NOTIFY:
		if (((LPNMHDR) lParam)->code == PSN_HELP) {
			ToggleKeyboard();
			return 0;
		}
		break;
	}

	return DefWindowProc(hDlg, uMsg, wParam, lParam);
}

BOOL SaveConfiguration8(HWND hDlg)
{
	int alphaThreshold;

	alphaThreshold = GetDlgItemInt(hDlg, IDC_EDIT_TRANS_THRESHOLD, NULL, TRUE);

	if (alphaThreshold < 0 || alphaThreshold > 255) {
		MessageBox(hDlg, TEXT("Alpha threshold value is not valid!"), TEXT("Error"), MB_OK);
		return FALSE;
	}

	configuracion->alphaThreshold = alphaThreshold;

	configuracion->alphaBlend = 0;
	if (SendMessage(GetDlgItem(hDlg, IDC_CHECK_TRANS_ALPHABLEND),   BM_GETCHECK, 0, 0) == BST_CHECKED) {
		configuracion->alphaBlend = 1;
	}
	if (SendMessage(GetDlgItem(hDlg, IDC_CHECK_TRANS_ALPHABLEND2),   BM_GETCHECK, 0, 0) == BST_CHECKED) {
		configuracion->alphaBlend = 2;
	}
	configuracion->alphaOnBlack   = SendMessage(GetDlgItem(hDlg, IDC_CHECK_TRANS_ALPHAONBLACK), BM_GETCHECK, 0, 0) == BST_CHECKED;
	configuracion->transparentBMP = SendMessage(GetDlgItem(hDlg, IDC_CHECK_TRANS_BMP),          BM_GETCHECK, 0, 0) == BST_CHECKED;

	return TRUE;
}
