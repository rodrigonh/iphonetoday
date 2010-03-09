//////////////////////////////////////////////////////////////////////////////
// PropertySheetPage1.cpp : Defines the property sheet page procedures.
//

#include "stdafx.h"
#include "iPhoneToday.h"
#include "OptionDialog.h"

void EnableAllDlgItems1(HWND hDlg, BOOL bEnable)
{
	EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BACK_WALLPAPER),		bEnable);
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_BACK_WALLPAPER),	bEnable);
	EnableWindow(GetDlgItem(hDlg, IDC_CHECK_BACK_STATIC),		bEnable);
	EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BACK_FACTOR),		bEnable);
	EnableWindow(GetDlgItem(hDlg, IDC_CHECK_BACK_FIT_WIDTH),	bEnable);
	EnableWindow(GetDlgItem(hDlg, IDC_CHECK_BACK_FIT_HEIGHT),	bEnable);
	EnableWindow(GetDlgItem(hDlg, IDC_CHECK_BACK_CENTER),		bEnable);
	EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BACK_COLOR),			bEnable);
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_BACK_COLOR),		bEnable);
}

/*************************************************************************/
/* General options dialog box procedure function                 */
/*************************************************************************/
LRESULT CALLBACK OptionDialog1(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			// Initialize handle to property sheet
			g_hDlg[1] = hDlg;

			SHINITDLGINFO shidi;

			// Create a Done button and size it.  
			shidi.dwMask = SHIDIM_FLAGS;
			shidi.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIZEDLG | SHIDIF_WANTSCROLLBAR;
			shidi.hDlg = hDlg;
			SHInitDialog(&shidi);

			SHInitExtraControls();

			if (configuracion == NULL) {
				configuracion = new CConfiguracion();
				configuracion->cargaXMLConfig();
			}
			if (configuracion != NULL) {
				SetDlgItemText(hDlg, IDC_EDIT_BACK_WALLPAPER, configuracion->strFondoPantalla);
				SendMessage(GetDlgItem(hDlg, IDC_CHECK_BACK_STATIC),		BM_SETCHECK, configuracion->fondoEstatico ? BST_CHECKED : BST_UNCHECKED, 0);
				SendMessage(GetDlgItem(hDlg, IDC_CHECK_BACK_FIT_WIDTH),		BM_SETCHECK, configuracion->fondoFitWidth ? BST_CHECKED : BST_UNCHECKED, 0);
				SendMessage(GetDlgItem(hDlg, IDC_CHECK_BACK_FIT_HEIGHT),	BM_SETCHECK, configuracion->fondoFitHeight ? BST_CHECKED : BST_UNCHECKED, 0);
				SendMessage(GetDlgItem(hDlg, IDC_CHECK_BACK_CENTER),		BM_SETCHECK, configuracion->fondoCenter ? BST_CHECKED : BST_UNCHECKED, 0);
				SendMessage(GetDlgItem(hDlg, IDC_CHECK_BACK_TRANSPARENT),	BM_SETCHECK, configuracion->fondoTransparente ? BST_CHECKED : BST_UNCHECKED, 0);
				SetDlgItemHex(hDlg, IDC_EDIT_BACK_COLOR, configuracion->fondoColor);

				SetDlgItemFloat(hDlg, IDC_EDIT_BACK_FACTOR, configuracion->fondoFactor);
				EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BACK_FACTOR), !(configuracion->fondoFitWidth || configuracion->fondoFitHeight));
#ifdef EXEC_MODE
				EnableWindow(GetDlgItem(hDlg, IDC_CHECK_BACK_TRANSPARENT), FALSE);
#else
				EnableAllDlgItems1(hDlg, !configuracion->fondoTransparente);
#endif
			} else {
				MessageBox(hDlg, L"Empty Configuration!", 0, MB_OK);
			}
		}
		return TRUE;
	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDC_CHECK_BACK_TRANSPARENT:
				EnableAllDlgItems1(hDlg, SendMessage(GetDlgItem(hDlg, IDC_CHECK_BACK_TRANSPARENT), BM_GETCHECK, 0, 0) == BST_UNCHECKED);
				break;
			case IDC_BUTTON_BACK_WALLPAPER:
				TCHAR str[MAX_PATH];
				TCHAR fullPath[MAX_PATH];
				TCHAR browseDir[MAX_PATH];
				GetDlgItemText(hDlg, IDC_EDIT_BACK_WALLPAPER, str, MAX_PATH);
				configuracion->getAbsolutePath(fullPath, MAX_PATH, str);
				getPathFromFile(fullPath, browseDir);
				if (openFileBrowse(hDlg, OFN_EXFLAG_THUMBNAILVIEW, str, browseDir)) {
					SetDlgItemText(hDlg, IDC_EDIT_BACK_WALLPAPER, str);
				}
				break;
			case IDC_BUTTON_BACK_COLOR:
				int rgbCurrent;
				COLORREF nextColor;
				rgbCurrent = GetDlgItemHex(hDlg, IDC_EDIT_BACK_COLOR, NULL);
				if (ColorSelector(hDlg, rgbCurrent, &nextColor)) {
					SetDlgItemHex(hDlg, IDC_EDIT_BACK_COLOR, nextColor);
				}
				break;
			case IDC_CHECK_BACK_FIT_WIDTH:
			case IDC_CHECK_BACK_FIT_HEIGHT:
				BOOL checked1 = SendMessage(GetDlgItem(hDlg, IDC_CHECK_BACK_FIT_WIDTH), BM_GETCHECK, 0, 0) == BST_CHECKED;
				BOOL checked2 = SendMessage(GetDlgItem(hDlg, IDC_CHECK_BACK_FIT_HEIGHT), BM_GETCHECK, 0, 0) == BST_CHECKED;
				EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BACK_FACTOR), !(checked1 || checked2));
				break;
			}
		}
		return 0;
	case WM_CTLCOLORSTATIC:
		return (LRESULT)GetStockObject(WHITE_BRUSH);
	}

	return DefWindowProc(hDlg, uMsg, wParam, lParam);
}

BOOL SaveConfiguration1(HWND hDlg)
{
	GetDlgItemText(hDlg, IDC_EDIT_BACK_WALLPAPER, configuracion->strFondoPantalla, CountOf(configuracion->strFondoPantalla));
	configuracion->fondoEstatico	= SendMessage(GetDlgItem(hDlg, IDC_CHECK_BACK_STATIC), BM_GETCHECK, 0, 0) == BST_CHECKED;
	configuracion->fondoFitWidth	= SendMessage(GetDlgItem(hDlg, IDC_CHECK_BACK_FIT_WIDTH), BM_GETCHECK, 0, 0) == BST_CHECKED;
	configuracion->fondoFitHeight	= SendMessage(GetDlgItem(hDlg, IDC_CHECK_BACK_FIT_HEIGHT), BM_GETCHECK, 0, 0) == BST_CHECKED;
	configuracion->fondoCenter		= SendMessage(GetDlgItem(hDlg, IDC_CHECK_BACK_CENTER), BM_GETCHECK, 0, 0) == BST_CHECKED;
	configuracion->fondoTransparente= SendMessage(GetDlgItem(hDlg, IDC_CHECK_BACK_TRANSPARENT), BM_GETCHECK, 0, 0) == BST_CHECKED;
	configuracion->fondoColor		= GetDlgItemHex(hDlg,   IDC_EDIT_BACK_COLOR, NULL);
	configuracion->fondoFactor		= GetDlgItemFloat(hDlg, IDC_EDIT_BACK_FACTOR, NULL);

	return TRUE;
}
