#include "stdafx.h"
#include "CConfiguracion.h"
#include "RegistryUtils.h"
#include "iPhoneToday.h"

CConfiguracion::CConfiguracion(void)
{
	// Initialize paths

	// Get the path of the executable
	TCHAR pathExecutable[MAX_PATH];
#ifdef EXEC_MODE
	GetModuleFileName(NULL, pathExecutable, CountOf(pathExecutable));
#else
	LoadTextSetting(HKEY_LOCAL_MACHINE, pathExecutable, L"Software\\Microsoft\\Today\\Items\\iPhoneToday", L"DLL", L"");
#endif

	StringCchCopy(pathExecutableDir, CountOf(pathExecutableDir), pathExecutable);
	TCHAR *p = wcsrchr(pathExecutableDir, '\\');
	if (p != NULL) *(p+1) = '\0';

	//p = L"\\iPhoneToday\\";
	//if (!FileOrDirExists(p, TRUE)) {
		p = pathExecutableDir;
	//}
	StringCchPrintf(pathSettingsXML, CountOf(pathSettingsXML),   L"%s%s", p, L"settings.xml");
	StringCchPrintf(pathIconsXML,    CountOf(pathIconsXML),      L"%s%s", p, L"icons.xml");
	StringCchPrintf(pathIconsDir,    CountOf(pathIconsDir),      L"%s%s", p, L"icons\\");

	StringCchCopy(pathIconsXMLDir, CountOf(pathIconsXMLDir), pathIconsXML);
	p = wcsrchr(pathIconsXMLDir, '\\');
	if (p != NULL) *(p+1) = '\0';

	fondoPantalla = NULL;
	mainScreenConfig = new CConfigurationScreen();
	bottomBarConfig = new CConfigurationScreen();
	topBarConfig = new CConfigurationScreen();
	altoPantallaMax = 0;

	this->defaultValues();
}

CConfiguracion::~CConfiguracion(void)
{
	if (pressedIcon != NULL) {
		delete pressedIcon;
	}
	if (bubbleNotif != NULL) {
		delete bubbleNotif;
	}
	if (bubbleState != NULL) {
		delete bubbleState;
	}
	if (bubbleAlarm != NULL) {
		delete bubbleAlarm;
	}
	if (fondoPantalla != NULL) {
		delete fondoPantalla;
	}
	if (mainScreenConfig != NULL) {
		delete mainScreenConfig;
	}
	if (bottomBarConfig != NULL) {
		delete bottomBarConfig;
	}
	if (topBarConfig != NULL) {
		delete topBarConfig;
	}
}

BOOL CConfiguracion::hasTimestampChanged()
{
	FILETIME newLastModifiedSettingsXML = FileModifyTime(pathSettingsXML);
	FILETIME newLastModifiedIconsXML = FileModifyTime(pathIconsXML);
	if (CompareFileTime(&newLastModifiedSettingsXML, &lastModifiedSettingsXML) != 0 ||
		CompareFileTime(&newLastModifiedIconsXML, &lastModifiedIconsXML) != 0) {
		return TRUE;
	}
	return FALSE;
}

void CConfiguracion::getAbsolutePath(LPTSTR pszDest, size_t cchDest, LPCTSTR pszSrc)
{
	// Convert relative path to absolute
	if (pszSrc[0] == L'\\') {
		StringCchCopy(pszDest, cchDest, pszSrc);
	} else {
		// First check whether it is relative to the Icons directory
		StringCchPrintf(pszDest, cchDest, L"%s%s", pathIconsDir, pszSrc);
		if (!FileExists(pszDest)) {
			// Second check whether it is relative to the icons.xml directory
			StringCchPrintf(pszDest, cchDest, L"%s%s", pathIconsXMLDir, pszSrc);
			if (!FileExists(pszDest)) {
				// Third check whether it is relative to the executable's directory
				StringCchPrintf(pszDest, cchDest, L"%s%s", pathExecutableDir, pszSrc);
			}
		}
	}
}

// maxIconos = Maximo de iconos que hay en una pantalla
void CConfiguracion::calculaConfiguracion(int maxIconos, int numIconsInBottomBar, int numIconsInTopBar, int width, int height)
{
	if (width > 0) {
		anchoPantalla = width;
	}

	if (height > 0) {
		altoPantalla = height;
	}

	bottomBarConfig->calculate(TRUE, numIconsInBottomBar, anchoPantalla, altoPantalla);
	topBarConfig->calculate(TRUE, numIconsInTopBar, anchoPantalla, altoPantalla);
	mainScreenConfig->calculate(FALSE, maxIconos, anchoPantalla, altoPantalla);

	altoPantallaMax = max(((maxIconos + mainScreenConfig->iconsPerRow - 1) / mainScreenConfig->iconsPerRow) * mainScreenConfig->distanceIconsV + mainScreenConfig->posReference.y * 2, altoPantalla);
}

BOOL CConfiguracion::cargaXMLIconos(CListaPantalla *listaPantallas)
{
	if (listaPantallas == NULL) {
		return FALSE;
	}

	TiXmlDocument doc;
	FILE *f = _wfopen(pathIconsXML, L"rb");
	if (!f) return FALSE;
	bool loaded = doc.LoadFile(f, TIXML_ENCODING_UTF8);
	fclose(f);
	if (!loaded) return FALSE;
	TiXmlNode* pRoot = doc.FirstChild("root");
	if (!pRoot) return FALSE;

	lastModifiedIconsXML = FileModifyTime(pathIconsXML);

	if (listaPantallas->barraInferior == NULL) {
		listaPantallas->barraInferior = new CPantalla();
	}
	if (listaPantallas->topBar == NULL) {
		listaPantallas->topBar = new CPantalla();
	}

	// for each screen (child of root)
	UINT nScreen = 0;
	for (TiXmlElement *pElemScreen = pRoot->FirstChildElement(); pElemScreen; pElemScreen = pElemScreen->NextSiblingElement()) {
		const char *nameNode = pElemScreen->Value();
		CPantalla *pantalla = NULL;
		if (_stricmp(nameNode, "BottomBar") == 0) {
			pantalla = listaPantallas->barraInferior;
		} else if (_stricmp(nameNode, "TopBar") == 0) {
			pantalla = listaPantallas->topBar;
		} else if (_stricmp(nameNode, "screen") == 0) {
			if (listaPantallas->listaPantalla[nScreen] == NULL) {
				pantalla = listaPantallas->creaPantalla();
			} else {
				pantalla = listaPantallas->listaPantalla[nScreen];
			}
			nScreen++;
		} else {
			continue;
		}

		XMLUtils::GetAttr(pElemScreen, "header", pantalla->header, CountOf(pantalla->header));

		// for each icon (child of screen)
		UINT nIcon = 0;
		for (TiXmlElement* pElemIcon = pElemScreen->FirstChildElement(); pElemIcon; pElemIcon = pElemIcon->NextSiblingElement()) {
			if (_stricmp(pElemIcon->Value(), "icon") == 0) {
				CIcono *icono;

				if (pantalla->listaIconos[nIcon] == NULL) {
					icono = pantalla->creaIcono();
				} else {
					icono = pantalla->listaIconos[nIcon];
					icono->defaultValues();
				}
				nIcon++;

				XMLUtils::GetAttr(pElemIcon, "name",			icono->nombre,			CountOf(icono->nombre));
				XMLUtils::GetAttr(pElemIcon, "image",			icono->rutaImagen,		CountOf(icono->rutaImagen));
				XMLUtils::GetAttr(pElemIcon, "sound",			icono->sound,			CountOf(icono->sound));
				XMLUtils::GetAttr(pElemIcon, "exec",			icono->ejecutable,		CountOf(icono->ejecutable));
				XMLUtils::GetAttr(pElemIcon, "parameters",		icono->parametros,		CountOf(icono->parametros));
				XMLUtils::GetAttr(pElemIcon, "execAlt",			icono->ejecutableAlt,	CountOf(icono->ejecutableAlt));
				XMLUtils::GetAttr(pElemIcon, "parametersAlt",	icono->parametrosAlt,	CountOf(icono->parametrosAlt));
				XMLUtils::GetAttr(pElemIcon, "type",			&icono->tipo);
				XMLUtils::GetAttr(pElemIcon, "animation",		&icono->launchAnimation);
			}
		}
		while (nIcon < pantalla->numIconos) {
			pantalla->borraIcono(nIcon);
		}
	}

	while (nScreen < listaPantallas->numPantallas) {
		listaPantallas->numPantallas--;
		delete listaPantallas->listaPantalla[listaPantallas->numPantallas];
	}

	return TRUE;
}

BOOL CConfiguracion::cargaXMLIconos2(CListaPantalla *listaPantallas)
{
	// long duration = -(long)GetTickCount();
	BOOL result = cargaXMLIconos(listaPantallas);
	// duration += GetTickCount();
	// NKDbgPrintfW(L" *** %d \t to cargaXMLIconos.\n", duration);

	if (result == false) {
#if EXEC_MODE
		MessageBox(g_hWnd, TEXT("Bad icons.xml, check for errors please. (Remember: No Special Characters and tag well formed)"), TEXT("Error"), MB_OK);
#endif
		if (listaPantallas->numPantallas == 0) {
			listaPantallas->creaPantalla();
		}
		if (listaPantallas->barraInferior == NULL) {
			listaPantallas->barraInferior = new CPantalla();
		}
		if (listaPantallas->topBar == NULL) {
			listaPantallas->topBar = new CPantalla();
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CConfiguracion::cargaIconsImages(HDC *hDC, CListaPantalla *listaPantallas)
{
	TIMER_RESET(loadImage_duration);
	TIMER_RESET(loadImage_load_duration);
	TIMER_RESET(loadImage_resize_duration);
	TIMER_RESET(loadImage_fix_duration);

	int nPantallas = listaPantallas->numPantallas;
	int nIconos;
	CPantalla *pantalla;
	CIcono *icono;

	for (int i = 0; i < nPantallas; i++) {
		pantalla = listaPantallas->listaPantalla[i];
		nIconos = pantalla->numIconos;
		for (int j = 0; j < nIconos; j++) {
			icono = pantalla->listaIconos[j];
			cargaImagenIcono(hDC, icono, MAINSCREEN);
		}
	}

	// Cargamos los iconos de la barra inferior
	pantalla = listaPantallas->barraInferior;

	nIconos = pantalla->numIconos;
	for (int j = 0; j < nIconos; j++) {
		icono = pantalla->listaIconos[j];
		cargaImagenIcono(hDC, icono, BOTTOMBAR);
	}

	// Load icons for the top bar
	pantalla = listaPantallas->topBar;

	nIconos = pantalla->numIconos;
	for (int j = 0; j < nIconos; j++) {
		icono = pantalla->listaIconos[j];
		cargaImagenIcono(hDC, icono, TOPBAR);
	}

#ifdef TIMING
	NKDbgPrintfW(L" *** %d msec\t loadImage.\n", loadImage_duration);	
	NKDbgPrintfW(L" *** %d msec\t loadImage_load_duration.\n", loadImage_load_duration);
	NKDbgPrintfW(L" *** %d msec\t loadImage_resize_duration.\n", loadImage_resize_duration);
	NKDbgPrintfW(L" *** %d msec\t loadImage_fix_duration.\n", loadImage_fix_duration);
#endif

	return TRUE;
}

BOOL CConfiguracion::cargaImagenIcono(HDC *hDC, CIcono *icono, SCREEN_TYPE screen_type)
{
	BOOL result;
	TCHAR rutaImgCompleta[MAX_PATH];
	UINT width;
	if (screen_type == BOTTOMBAR) {
		width = bottomBarConfig->iconWidth;
	} else if (screen_type == TOPBAR) {
		width = topBarConfig->iconWidth;
	} else {
		width = mainScreenConfig->iconWidth;
	}

	if (icono->rutaImagen != NULL && _tcslen(icono->rutaImagen) > 0) {
		getAbsolutePath(rutaImgCompleta, CountOf(rutaImgCompleta), icono->rutaImagen);
		icono->loadImage(hDC, rutaImgCompleta, width, width);
	} else if (icono->ejecutable != NULL && _tcslen(icono->ejecutable) > 0) {
		getAbsolutePath(rutaImgCompleta, CountOf(rutaImgCompleta), icono->ejecutable);
		icono->loadImageFromExec(hDC, rutaImgCompleta, width, width);
	}

	result = true;
	return result;
}
BOOL CConfiguracion::cargaFondo(HDC *hDC)
{
#ifndef EXEC_MODE
	BOOL isTransparent = this->fondoTransparente;
	if (!isTransparent) {
#endif
		if (fondoPantalla != NULL) {
			delete fondoPantalla;
		}
		fondoPantalla = new CIcono();

		TCHAR rutaImgCompleta[MAX_PATH];
		getAbsolutePath(rutaImgCompleta, CountOf(rutaImgCompleta), strFondoPantalla);
		fondoPantalla->loadImage(hDC, rutaImgCompleta, this->fondoFitWidth ? anchoPantalla : 0, this->fondoFitHeight ? altoPantalla : 0, PIXFMT_16BPP_RGB565, this->fondoFactor);
		if (fondoPantalla->hDC == NULL) {
			delete fondoPantalla;
			fondoPantalla = NULL;
		}
#ifndef EXEC_MODE
	} else {
		if (fondoPantalla != NULL) {
			delete fondoPantalla;
			fondoPantalla = NULL;
		}
	}
#endif
	return TRUE;
}

BOOL CConfiguracion::cargaImagenes(HDC *hDC)
{
	BOOL result = false;
	TCHAR rutaImgCompleta[MAX_PATH];

	// Background
	cargaFondo(hDC);

	// Pressed icon
	pressedIcon = new CIcono();
	getAbsolutePath(rutaImgCompleta, CountOf(rutaImgCompleta), pressed_icon);
	pressedIcon->loadImage(hDC, rutaImgCompleta, mainScreenConfig->iconWidth, mainScreenConfig->iconWidth, PIXFMT_32BPP_ARGB, 1, TRUE);

	// Bubbles
	bubbleNotif = new CIcono();
	getAbsolutePath(rutaImgCompleta, CountOf(rutaImgCompleta), bubble_notif);
	bubbleNotif->loadImage(hDC, rutaImgCompleta, UINT(mainScreenConfig->iconWidth * 0.80), UINT(mainScreenConfig->iconWidth * 0.80));

	bubbleState = new CIcono();
	getAbsolutePath(rutaImgCompleta, CountOf(rutaImgCompleta), bubble_state);
	bubbleState->loadImage(hDC, rutaImgCompleta, mainScreenConfig->iconWidth, UINT(mainScreenConfig->iconWidth * 0.50));

	bubbleAlarm = new CIcono();
	getAbsolutePath(rutaImgCompleta, CountOf(rutaImgCompleta), bubble_alarm);
	bubbleAlarm->loadImage(hDC, rutaImgCompleta, UINT(mainScreenConfig->iconWidth * 0.80), UINT(mainScreenConfig->iconWidth * 0.80));

	result = true;
	return result;
}

void CConfiguracion::defaultValues()
{
	this->mainScreenConfig->defaultValues();
	this->bottomBarConfig->defaultValues();
	this->topBarConfig->defaultValues();

	this->circlesDiameter = 15;
	this->circlesDistance = 7;
	this->circlesOffset = 7;

	this->headerFontSize = 0;
	this->headerFontWeight = 900;
	this->headerFontColor = RGB(255, 255, 255);
	this->headerOffset = 0;

	this->fondoTransparente = 1;
	this->fondoColor = RGB(0, 0, 0);
	this->fondoEstatico = 1;
	this->fondoFitWidth = 1;
	this->fondoFitHeight = 1;
	this->fondoCenter = 1;
	this->fondoFactor = 1;
	StringCchCopy(this->strFondoPantalla, CountOf(this->strFondoPantalla), L"");

	this->umbralMovimiento = 15;
	this->velMaxima = 140;
	this->velMinima = 20;
	this->refreshTime = 20;
	this->factorMovimiento = 4;
	this->verticalScroll = 0;
	this->freestyleScroll = 0;

	this->dowUseLocale = 1;
	StringCchCopy(this->diasSemana[0], CountOf(this->diasSemana[0]), TEXT("Sun"));
	StringCchCopy(this->diasSemana[1], CountOf(this->diasSemana[1]), TEXT("Mon"));
	StringCchCopy(this->diasSemana[2], CountOf(this->diasSemana[2]), TEXT("Tue"));
	StringCchCopy(this->diasSemana[3], CountOf(this->diasSemana[3]), TEXT("Wed"));
	StringCchCopy(this->diasSemana[4], CountOf(this->diasSemana[4]), TEXT("Thu"));
	StringCchCopy(this->diasSemana[5], CountOf(this->diasSemana[5]), TEXT("Fri"));
	StringCchCopy(this->diasSemana[6], CountOf(this->diasSemana[6]), TEXT("Sat"));
	StringCchCopy(this->strFondoPantalla, CountOf(this->strFondoPantalla), TEXT(""));

	this->dow.color = RGB(255,255,255);
	this->dow.width = 18;
	this->dow.height = 40;
	this->dow.weight = 400;
	this->dow.offset.left = 0;
	this->dow.offset.top = 0;
	this->dow.offset.right = 0;
	this->dow.offset.bottom = 72;

	this->dom.color = RGB(30,30,30);
	this->dom.width = 30;
	this->dom.height = 80;
	this->dom.weight = 800;
	this->dom.offset.left = 0;
	this->dom.offset.top = 25;
	this->dom.offset.right = 0;
	this->dom.offset.bottom = 0;

	this->clck.color = RGB(230,230,230);
	this->clck.width = 13;
	this->clck.height = 60;
	this->clck.weight = 900;
	this->clck.offset.left = 0;
	this->clck.offset.top = 0;
	this->clck.offset.right = 0;
	this->clck.offset.bottom = 0;

	this->clock12Format = 0;

	this->batt.color = RGB(230,230,230);
	this->batt.width = 25;
	this->batt.height = 70;
	this->batt.weight = 900;
	this->batt.offset.left = 0;
	this->batt.offset.top = 0;
	this->batt.offset.right = 0;
	this->batt.offset.bottom = 0;

	this->vol.color = RGB(0,64,128);
	this->vol.width = 25;
	this->vol.height = 70;
	this->vol.weight = 900;
	this->vol.offset.left = 0;
	this->vol.offset.top = 0;
	this->vol.offset.right = 0;
	this->vol.offset.bottom = 0;

	this->meml.color = RGB(230,230,230);
	this->meml.width = 25;
	this->meml.height = 70;
	this->meml.weight = 900;
	this->meml.offset.left = 0;
	this->meml.offset.top = 0;
	this->meml.offset.right = 0;
	this->meml.offset.bottom = 0;

	this->memf.color = RGB(230,230,230);
	this->memf.width = 20;
	this->memf.height = 70;
	this->memf.weight = 600;
	this->memf.offset.left = 0;
	this->memf.offset.top = 0;
	this->memf.offset.right = 0;
	this->memf.offset.bottom = 0;

	this->memu.color = RGB(230,230,230);
	this->memu.width = 20;
	this->memu.height = 70;
	this->memu.weight = 600;
	this->memu.offset.left = 0;
	this->memu.offset.top = 0;
	this->memu.offset.right = 0;
	this->memu.offset.bottom = 0;

	this->sign.color = RGB(230,230,230);
	this->sign.width = 25;
	this->sign.height = 70;
	this->sign.weight = 900;
	this->sign.offset.left = 0;
	this->sign.offset.top = 0;
	this->sign.offset.right = 0;
	this->sign.offset.bottom = 0;

	StringCchCopy(this->bubble_notif, CountOf(this->bubble_notif), TEXT("bubble_notif.png"));
	StringCchCopy(this->bubble_state, CountOf(this->bubble_notif), TEXT("bubble_state.png"));
	StringCchCopy(this->bubble_alarm, CountOf(this->bubble_notif), TEXT("bubble_alarm.png"));

	this->closeOnLaunchIcon = 0;
	this->vibrateOnLaunchIcon = 40;
	this->allowAnimationOnLaunchIcon = 1;
	this->colorOfAnimationOnLaunchIcon = RGB(255,255,255);
	this->allowSoundOnLaunchIcon = 1;
	this->soundOnLaunchIcon[0] = 0;

	StringCchCopy(this->pressed_icon, CountOf(this->pressed_icon), TEXT("Pressed\\RoundedPressed.png"));
	this->pressed_sound[0] = 0;

	this->notifyTimer = 2000;
	this->ignoreRotation = 0;
	this->disableRightClick = 0;
	this->disableRightClickDots = 0;
	this->fullscreen = 0;
	this->neverShowTaskBar = 0;
	this->noWindowTitle = 0;
	this->showExit = 1;
	this->heightP = 0;
	this->heightL = 0;

	this->outOfScreenLeft[0] = 0;
	this->outOfScreenRight[0] = 0;
	this->outOfScreenTop[0] = 0;
	this->outOfScreenBottom[0] = 0;

	this->alphaBlend = 0;
	this->alphaOnBlack = 0;
	this->alphaThreshold = 25;
	this->transparentBMP = 1;

	this->alreadyConfigured = 0;

	if (isPND()) {
		this->disableRightClickDots = 1;
		this->vibrateOnLaunchIcon = 0;
		this->fullscreen = 1;
	}
}

void SpecialIconSettingsLoad(TiXmlElement *pElem, SpecialIconSettings *sis)
{
	XMLUtils::GetAttr(pElem, "color",  &sis->color);
	XMLUtils::GetAttr(pElem, "width",  &sis->width);
	XMLUtils::GetAttr(pElem, "height", &sis->height);
	XMLUtils::GetAttr(pElem, "weight", &sis->weight);
	XMLUtils::GetAttr(pElem, "left",   &sis->offset.left);
	XMLUtils::GetAttr(pElem, "top",    &sis->offset.top);
	XMLUtils::GetAttr(pElem, "right",  &sis->offset.right);
	XMLUtils::GetAttr(pElem, "bottom", &sis->offset.bottom);
}

void SpecialIconSettingsSave(TiXmlElement *pElem, SpecialIconSettings *sis)
{
	XMLUtils::SetAttr(pElem, "color",  sis->color);
	XMLUtils::SetAttr(pElem, "width",  sis->width);
	XMLUtils::SetAttr(pElem, "height", sis->height);
	XMLUtils::SetAttr(pElem, "weight", sis->weight);
	XMLUtils::SetAttr(pElem, "left",   sis->offset.left);
	XMLUtils::SetAttr(pElem, "top",    sis->offset.top);
	XMLUtils::SetAttr(pElem, "right",  sis->offset.right);
	XMLUtils::SetAttr(pElem, "bottom", sis->offset.bottom);
}

BOOL CConfiguracion::cargaXMLConfig()
{
	TiXmlDocument doc;
	FILE *f = _wfopen(pathSettingsXML, L"rb");
	if (!f) return FALSE;
	bool loaded = doc.LoadFile(f, TIXML_ENCODING_UTF8);
	fclose(f);
	if (!loaded) return FALSE;
	TiXmlNode* pRoot = doc.FirstChild("root");
	if (!pRoot) return FALSE;

	lastModifiedSettingsXML = FileModifyTime(pathSettingsXML);

	// for each child of root
	for (TiXmlElement *pElem = pRoot->FirstChildElement(); pElem; pElem = pElem->NextSiblingElement()) {
		const char *nameNode = pElem->Value();

		if(_stricmp(nameNode, "Circles") == 0) {
			XMLUtils::GetAttr(pElem, "diameter", &this->circlesDiameter);
			XMLUtils::GetAttr(pElem, "distance", &this->circlesDistance);
			XMLUtils::GetAttr(pElem, "offset", &this->circlesOffset);
		} else if(_stricmp(nameNode, "Header") == 0) {
			XMLUtils::GetAttr(pElem, "size",   &this->headerFontSize);
			XMLUtils::GetAttr(pElem, "color",  &this->headerFontColor);
			XMLUtils::GetAttr(pElem, "weight", &this->headerFontWeight);
			XMLUtils::GetAttr(pElem, "offset", &this->headerOffset);
		} else if(_stricmp(nameNode, "Movement") == 0) {
			XMLUtils::GetAttr(pElem, "MoveThreshold",  &this->umbralMovimiento);
			XMLUtils::GetAttr(pElem, "MaxVelocity",    &this->velMaxima);
			XMLUtils::GetAttr(pElem, "MinVelocity",    &this->velMinima);
			XMLUtils::GetAttr(pElem, "RefreshTime",    &this->refreshTime);
			XMLUtils::GetAttr(pElem, "FactorMov",      &this->factorMovimiento);
			XMLUtils::GetAttr(pElem, "VerticalScroll", &this->verticalScroll);
			XMLUtils::GetAttr(pElem, "FreestyleScroll", &this->freestyleScroll);
		} else if(_stricmp(nameNode, "DayOfWeek") == 0) {
			SpecialIconSettingsLoad(pElem, &this->dow);
			XMLUtils::GetAttr(pElem, "UseLocale", &dowUseLocale);
			XMLUtils::GetAttr(pElem, "Sunday",    this->diasSemana[0], CountOf(this->diasSemana[0]));
			XMLUtils::GetAttr(pElem, "Monday",    this->diasSemana[1], CountOf(this->diasSemana[1]));
			XMLUtils::GetAttr(pElem, "Tuesday",   this->diasSemana[2], CountOf(this->diasSemana[2]));
			XMLUtils::GetAttr(pElem, "Wednesday", this->diasSemana[3], CountOf(this->diasSemana[3]));
			XMLUtils::GetAttr(pElem, "Thursday",  this->diasSemana[4], CountOf(this->diasSemana[4]));
			XMLUtils::GetAttr(pElem, "Friday",    this->diasSemana[5], CountOf(this->diasSemana[5]));
			XMLUtils::GetAttr(pElem, "Saturday",  this->diasSemana[6], CountOf(this->diasSemana[6]));
		} else if(_stricmp(nameNode, "DayOfMonth") == 0) {
			SpecialIconSettingsLoad(pElem, &this->dom);
		} else if(_stricmp(nameNode, "Clock") == 0) {
			SpecialIconSettingsLoad(pElem, &this->clck);
			XMLUtils::GetAttr(pElem, "format12", &this->clock12Format);
		} else if(_stricmp(nameNode, "Battery") == 0) {
			SpecialIconSettingsLoad(pElem, &this->batt);
		} else if(_stricmp(nameNode, "Volume") == 0) {
			SpecialIconSettingsLoad(pElem, &this->vol);
		} else if(_stricmp(nameNode, "MemoryLoad") == 0) {
			SpecialIconSettingsLoad(pElem, &this->meml);
		} else if(_stricmp(nameNode, "MemoryFree") == 0) {
			SpecialIconSettingsLoad(pElem, &this->memf);
		} else if(_stricmp(nameNode, "MemoryUsed") == 0) {
			SpecialIconSettingsLoad(pElem, &this->memu);
		} else if(_stricmp(nameNode, "SignalStrength") == 0) {
			SpecialIconSettingsLoad(pElem, &this->sign);
		} else if(_stricmp(nameNode, "Bubbles") == 0) {
			XMLUtils::GetAttr(pElem, "notif", this->bubble_notif, CountOf(this->bubble_notif));
			XMLUtils::GetAttr(pElem, "state", this->bubble_state, CountOf(this->bubble_state));
			XMLUtils::GetAttr(pElem, "alarm", this->bubble_alarm, CountOf(this->bubble_alarm));
		} else if(_stricmp(nameNode, "OnLaunchIcon") == 0) {
			XMLUtils::GetAttr(pElem, "close",   &this->closeOnLaunchIcon);
			XMLUtils::GetAttr(pElem, "vibrate", &this->vibrateOnLaunchIcon);
			XMLUtils::GetAttr(pElem, "animate", &this->allowAnimationOnLaunchIcon);
			XMLUtils::GetAttr(pElem, "color",   &this->colorOfAnimationOnLaunchIcon);
			XMLUtils::GetAttr(pElem, "sound",   &this->allowSoundOnLaunchIcon);
			XMLUtils::GetAttr(pElem, "wav",     this->soundOnLaunchIcon, CountOf(this->soundOnLaunchIcon));
		} else if(_stricmp(nameNode, "OnPressIcon") == 0) {
			XMLUtils::GetAttr(pElem, "icon",  this->pressed_icon,  CountOf(this->pressed_icon));
			XMLUtils::GetAttr(pElem, "sound", this->pressed_sound, CountOf(this->pressed_sound));
		} else if(_stricmp(nameNode, "Background") == 0) {
			XMLUtils::GetAttr(pElem, "transparent", &this->fondoTransparente);
			XMLUtils::GetAttr(pElem, "color",       &this->fondoColor);
			XMLUtils::GetAttr(pElem, "static",      &this->fondoEstatico);
			XMLUtils::GetAttr(pElem, "fitwidth",    &this->fondoFitWidth);
			XMLUtils::GetAttr(pElem, "fitheight",   &this->fondoFitHeight);
			XMLUtils::GetAttr(pElem, "center",      &this->fondoCenter);
			XMLUtils::GetAttr(pElem, "factor",      &this->fondoFactor);
			XMLUtils::GetAttr(pElem, "wallpaper",   this->strFondoPantalla, CountOf(this->strFondoPantalla));
		} else if(_stricmp(nameNode, "NotifyTimer") == 0) {
			XMLUtils::GetTextElem(pElem, &this->notifyTimer);
		} else if(_stricmp(nameNode, "IgnoreRotation") == 0) {
			XMLUtils::GetTextElem(pElem, &this->ignoreRotation);
		} else if(_stricmp(nameNode, "DisableRightClick") == 0) {
			XMLUtils::GetTextElem(pElem, &this->disableRightClick);
		} else if(_stricmp(nameNode, "DisableRightClickDots") == 0) {
			XMLUtils::GetTextElem(pElem, &this->disableRightClickDots);
		} else if(_stricmp(nameNode, "Fullscreen") == 0) {
			XMLUtils::GetTextElem(pElem, &this->fullscreen);
		} else if(_stricmp(nameNode, "NeverShowTaskBar") == 0) {
			XMLUtils::GetTextElem(pElem, &this->neverShowTaskBar);
		} else if(_stricmp(nameNode, "NoWindowTitle") == 0) {
			XMLUtils::GetTextElem(pElem, &this->noWindowTitle);
		} else if(_stricmp(nameNode, "ShowExit") == 0) {
			XMLUtils::GetTextElem(pElem, &this->showExit);
		} else if(_stricmp(nameNode, "OutOfScreen") == 0) {
			XMLUtils::GetAttr(pElem, "left",   this->outOfScreenLeft,   CountOf(this->outOfScreenLeft));
			XMLUtils::GetAttr(pElem, "right",  this->outOfScreenRight,  CountOf(this->outOfScreenRight));
			XMLUtils::GetAttr(pElem, "top",    this->outOfScreenTop,    CountOf(this->outOfScreenTop));
			XMLUtils::GetAttr(pElem, "bottom", this->outOfScreenBottom, CountOf(this->outOfScreenBottom));
		} else if(_stricmp(nameNode, "Transparency") == 0) {
			XMLUtils::GetAttr(pElem, "alphablend",     &this->alphaBlend);
			XMLUtils::GetAttr(pElem, "alphaonblack",   &this->alphaOnBlack);
			XMLUtils::GetAttr(pElem, "threshold",      &this->alphaThreshold);
			XMLUtils::GetAttr(pElem, "transparentbmp", &this->transparentBMP);
		} else if(_stricmp(nameNode, "TodayItemHeight") == 0) {
			XMLUtils::GetAttr(pElem, "portrait",  &this->heightP);
			XMLUtils::GetAttr(pElem, "landscape", &this->heightL);
		} else if(_stricmp(nameNode, "AlreadyConfigured") == 0) {
			XMLUtils::GetTextElem(pElem, &this->alreadyConfigured);
		} else if(_stricmp(nameNode, "MainScreen") == 0) {
			mainScreenConfig->loadXMLConfig(pElem);
		} else if(_stricmp(nameNode, "BottomBar") == 0) {
			bottomBarConfig->loadXMLConfig(pElem);
		} else if(_stricmp(nameNode, "TopBar") == 0) {
			topBarConfig->loadXMLConfig(pElem);
		}
    }

	return TRUE;
}

BOOL CConfiguracion::guardaXMLIconos(CListaPantalla *listaPantallas)
{
	if (listaPantallas == NULL) {
		return FALSE;
	}

	TiXmlDocument doc;

	TiXmlDeclaration *decl = new TiXmlDeclaration("1.0", "UTF-8", "yes");
	doc.LinkEndChild(decl);

	TiXmlComment *comment = new TiXmlComment("iPhoneToday for Windows Mobile");
	doc.LinkEndChild(comment);

	TiXmlElement *root = new TiXmlElement("root");
	doc.LinkEndChild(root);

	CPantalla *pantalla;
	TiXmlElement *pElemScreen;

	for (UINT i = 0; i < listaPantallas->numPantallas; i++) {
		pantalla = listaPantallas->listaPantalla[i];

		pElemScreen = new TiXmlElement("screen");
		XMLUtils::SetAttr(pElemScreen, "header", pantalla->header, CountOf(pantalla->header));
		saveXMLScreenIcons(pElemScreen, pantalla);
		root->LinkEndChild(pElemScreen);
	}

	if (listaPantallas->barraInferior != NULL && listaPantallas->barraInferior->numIconos > 0) {
		pantalla = listaPantallas->barraInferior;

		pElemScreen = new TiXmlElement("BottomBar");
		saveXMLScreenIcons(pElemScreen, pantalla);
		root->LinkEndChild(pElemScreen);
	}

	if (listaPantallas->topBar != NULL && listaPantallas->topBar->numIconos > 0) {
		pantalla = listaPantallas->topBar;

		pElemScreen = new TiXmlElement("TopBar");
		saveXMLScreenIcons(pElemScreen, pantalla);
		root->LinkEndChild(pElemScreen);
	}

	FILE *f = _wfopen(pathIconsXML, L"wb");
	doc.SaveFile(f);
	fclose(f);

	lastModifiedIconsXML = FileModifyTime(pathIconsXML);

	return 0;
}

BOOL CConfiguracion::saveXMLScreenIcons(TiXmlElement *pElemScreen, CPantalla *pantalla)
{
	if (pantalla == NULL) {
		return FALSE;
	}

	for (UINT j = 0; j < pantalla->numIconos; j++) {
		CIcono *icon = pantalla->listaIconos[j];
		if (icon == NULL) continue;

		TiXmlElement *pElemIcon = new TiXmlElement("icon");

		XMLUtils::SetAttr(pElemIcon, "name",			icon->nombre,			CountOf(icon->nombre));
		XMLUtils::SetAttr(pElemIcon, "image",			icon->rutaImagen,		CountOf(icon->rutaImagen));
		XMLUtils::SetAttr(pElemIcon, "sound",			icon->sound,			CountOf(icon->sound));
		XMLUtils::SetAttr(pElemIcon, "exec",			icon->ejecutable,		CountOf(icon->ejecutable));
		XMLUtils::SetAttr(pElemIcon, "parameters",		icon->parametros,		CountOf(icon->parametros));
		XMLUtils::SetAttr(pElemIcon, "execAlt",			icon->ejecutableAlt,	CountOf(icon->ejecutableAlt));
		XMLUtils::SetAttr(pElemIcon, "parametersAlt",	icon->parametrosAlt,	CountOf(icon->parametrosAlt));
		XMLUtils::SetAttr(pElemIcon, "type",			icon->tipo);
		XMLUtils::SetAttr(pElemIcon, "animation",		icon->launchAnimation);

		pElemScreen->LinkEndChild(pElemIcon);
	}

	return TRUE;
}

BOOL CConfiguracion::guardaXMLConfig()
{
	TiXmlDocument doc;

	TiXmlDeclaration *decl = new TiXmlDeclaration("1.0", "UTF-8", "yes");
	doc.LinkEndChild(decl);

	TiXmlComment *comment = new TiXmlComment("iPhoneToday for Windows Mobile");
	doc.LinkEndChild(comment);

	TiXmlElement *root = new TiXmlElement("root");
	doc.LinkEndChild(root);

	TiXmlElement *pElem;

	pElem = new TiXmlElement("MainScreen");
	mainScreenConfig->saveXMLConfig(pElem);
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("BottomBar");
	bottomBarConfig->saveXMLConfig(pElem);
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("TopBar");
	topBarConfig->saveXMLConfig(pElem);
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("Circles");
	XMLUtils::SetAttr(pElem, "diameter", this->circlesDiameter);
	XMLUtils::SetAttr(pElem, "distance", this->circlesDistance);
	XMLUtils::SetAttr(pElem, "offset",   this->circlesOffset);
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("Header");
	XMLUtils::SetAttr(pElem, "size",   this->headerFontSize);
	XMLUtils::SetAttr(pElem, "color",  this->headerFontColor);
	XMLUtils::SetAttr(pElem, "weight", this->headerFontWeight);
	XMLUtils::SetAttr(pElem, "offset", this->headerOffset);
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("Background");
	XMLUtils::SetAttr(pElem, "transparent", this->fondoTransparente);
	XMLUtils::SetAttr(pElem, "color",       this->fondoColor);
	XMLUtils::SetAttr(pElem, "static",      this->fondoEstatico);
	XMLUtils::SetAttr(pElem, "fitwidth",    this->fondoFitWidth);
	XMLUtils::SetAttr(pElem, "fitheight",   this->fondoFitHeight);
	XMLUtils::SetAttr(pElem, "center",      this->fondoCenter);
	XMLUtils::SetAttr(pElem, "factor",      this->fondoFactor);
	XMLUtils::SetAttr(pElem, "wallpaper",   this->strFondoPantalla, CountOf(this->strFondoPantalla));
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("Movement");
	XMLUtils::SetAttr(pElem, "MoveThreshold",  this->umbralMovimiento);
	XMLUtils::SetAttr(pElem, "MaxVelocity",    this->velMaxima);
	XMLUtils::SetAttr(pElem, "MinVelocity",    this->velMinima);
	XMLUtils::SetAttr(pElem, "RefreshTime",    this->refreshTime);
	XMLUtils::SetAttr(pElem, "FactorMov",      this->factorMovimiento);
	XMLUtils::SetAttr(pElem, "VerticalScroll", this->verticalScroll);
	XMLUtils::SetAttr(pElem, "FreestyleScroll", this->freestyleScroll);
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("DayOfWeek");
	SpecialIconSettingsSave(pElem, &this->dow);
	XMLUtils::SetAttr(pElem, "UseLocale", dowUseLocale);
	XMLUtils::SetAttr(pElem, "Sunday",    this->diasSemana[0], CountOf(this->diasSemana[0]));
	XMLUtils::SetAttr(pElem, "Monday",    this->diasSemana[1], CountOf(this->diasSemana[1]));
	XMLUtils::SetAttr(pElem, "Tuesday",   this->diasSemana[2], CountOf(this->diasSemana[2]));
	XMLUtils::SetAttr(pElem, "Wednesday", this->diasSemana[3], CountOf(this->diasSemana[3]));
	XMLUtils::SetAttr(pElem, "Thursday",  this->diasSemana[4], CountOf(this->diasSemana[4]));
	XMLUtils::SetAttr(pElem, "Friday",    this->diasSemana[5], CountOf(this->diasSemana[5]));
	XMLUtils::SetAttr(pElem, "Saturday",  this->diasSemana[6], CountOf(this->diasSemana[6]));
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("DayOfMonth");
	SpecialIconSettingsSave(pElem, &this->dom);
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("Clock");
	SpecialIconSettingsSave(pElem, &this->clck);
	XMLUtils::SetAttr(pElem, "format12", this->clock12Format);
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("Battery");
	SpecialIconSettingsSave(pElem, &this->batt);
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("Volume");
	SpecialIconSettingsSave(pElem, &this->vol);
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("MemoryLoad");
	SpecialIconSettingsSave(pElem, &this->meml);
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("MemoryFree");
	SpecialIconSettingsSave(pElem, &this->memf);
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("MemoryUsed");
	SpecialIconSettingsSave(pElem, &this->memu);
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("SignalStrength");
	SpecialIconSettingsSave(pElem, &this->sign);
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("Bubbles");
	XMLUtils::SetAttr(pElem, "notif", this->bubble_notif, CountOf(this->bubble_notif));
	XMLUtils::SetAttr(pElem, "state", this->bubble_state, CountOf(this->bubble_state));
	XMLUtils::SetAttr(pElem, "alarm", this->bubble_alarm, CountOf(this->bubble_alarm));
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("OnLaunchIcon");
	XMLUtils::SetAttr(pElem, "close",   this->closeOnLaunchIcon);
	XMLUtils::SetAttr(pElem, "vibrate", this->vibrateOnLaunchIcon);
	XMLUtils::SetAttr(pElem, "animate", this->allowAnimationOnLaunchIcon);
	XMLUtils::SetAttr(pElem, "color",   this->colorOfAnimationOnLaunchIcon);
	XMLUtils::SetAttr(pElem, "sound",   this->allowSoundOnLaunchIcon);
	XMLUtils::SetAttr(pElem, "wav",     this->soundOnLaunchIcon, CountOf(this->soundOnLaunchIcon));
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("OnPressIcon");
	XMLUtils::SetAttr(pElem, "icon",  this->pressed_icon,  CountOf(this->pressed_icon));
	XMLUtils::SetAttr(pElem, "sound", this->pressed_sound, CountOf(this->pressed_sound));
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("NotifyTimer");
	XMLUtils::SetTextElem(pElem, this->notifyTimer);
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("IgnoreRotation");
	XMLUtils::SetTextElem(pElem, this->ignoreRotation);
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("DisableRightClick");
	XMLUtils::SetTextElem(pElem, this->disableRightClick);
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("DisableRightClickDots");
	XMLUtils::SetTextElem(pElem, this->disableRightClickDots);
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("Fullscreen");
	XMLUtils::SetTextElem(pElem, this->fullscreen);
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("NeverShowTaskBar");
	XMLUtils::SetTextElem(pElem, this->neverShowTaskBar);
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("NoWindowTitle");
	XMLUtils::SetTextElem(pElem, this->noWindowTitle);
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("ShowExit");
	XMLUtils::SetTextElem(pElem, this->showExit);
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("OutOfScreen");
	XMLUtils::SetAttr(pElem, "left",   this->outOfScreenLeft,   CountOf(this->outOfScreenLeft));
	XMLUtils::SetAttr(pElem, "right",  this->outOfScreenRight,  CountOf(this->outOfScreenRight));
	XMLUtils::SetAttr(pElem, "top",    this->outOfScreenTop,    CountOf(this->outOfScreenTop));
	XMLUtils::SetAttr(pElem, "bottom", this->outOfScreenBottom, CountOf(this->outOfScreenBottom));
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("Transparency");
	XMLUtils::SetAttr(pElem, "alphablend",     this->alphaBlend);
	XMLUtils::SetAttr(pElem, "alphaonblack",   this->alphaOnBlack);
	XMLUtils::SetAttr(pElem, "threshold",      this->alphaThreshold);
	XMLUtils::SetAttr(pElem, "transparentbmp", this->transparentBMP);
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("TodayItemHeight");
	XMLUtils::SetAttr(pElem, "portrait",  this->heightP);
	XMLUtils::SetAttr(pElem, "landscape", this->heightL);
	root->LinkEndChild(pElem);

	pElem = new TiXmlElement("AlreadyConfigured");
	XMLUtils::SetTextElem(pElem, this->alreadyConfigured);
	root->LinkEndChild(pElem);

	FILE *f = _wfopen(pathSettingsXML, L"wb");
	doc.SaveFile(f);
	fclose(f);

	lastModifiedSettingsXML = FileModifyTime(pathSettingsXML);

	return 0;
}

void CConfiguracion::autoConfigure()
{
	int width = GetSystemMetrics(SM_CXSCREEN);
	int height = GetSystemMetrics(SM_CYSCREEN);
	int tmp;
	if (width > height) {
		tmp = height;
		height = width;
		width = tmp;
	}

	int iconWidth = int(float(width) * 0.1875);
	int fontSize = iconWidth / 4;

	this->mainScreenConfig->cs.iconWidthXML = iconWidth;
	this->mainScreenConfig->cs.fontSize = fontSize;

	this->bottomBarConfig->cs.iconWidthXML = iconWidth;
	this->bottomBarConfig->cs.fontSize = fontSize;

	this->topBarConfig->cs.iconWidthXML = iconWidth;
	this->topBarConfig->cs.fontSize = fontSize;

	this->mainScreenConfig->cs.minHorizontalSpace = 5;
	this->mainScreenConfig->cs.offset.top = 5;
	this->bottomBarConfig->cs.offset.top = this->mainScreenConfig->cs.offset.top;

	this->circlesDiameter = iconWidth / 6;
	this->circlesDistance = this->circlesDiameter / 2;
	this->circlesOffset = this->circlesDistance;

	this->alreadyConfigured = 1;
}
