;-------------------------------------------------
; ��װ����
;-------------------------------------------------
Name "HaoYin CheckGenTool"
Caption "HaoYin CheckGenTool"
Icon "..\00 Install Base\install.ico"
UninstallIcon "..\00 Install Base\uninstall.ico"
InstallDir "D:\Program Files\HaoYin_CheckGenTool"
OutFile "HYCheckGenAutoSetup_v1.0.exe"

RequestExecutionLevel admin

SilentInstall normal
SilentUninstall normal
AutoCloseWindow false
ShowInstDetails show
ShowUninstDetails hide
BrandingText "http://www.avcon.com.cn"

;--------------------------------
; ����
;--------------------------------
LoadLanguageFile "${NSISDIR}\Contrib\Language files\English.nlf"
LoadLanguageFile "${NSISDIR}\Contrib\Language files\TradChinese.nlf"
LoadLanguageFile "${NSISDIR}\Contrib\Language files\SimpChinese.nlf"
LoadLanguageFile "${NSISDIR}\Contrib\Language files\Japanese.nlf"

LicenseLangString myLicenseData ${LANG_ENGLISH} "..\00 Install Base\license-en.txt"
LicenseLangString myLicenseData ${LANG_TRADCHINESE} "..\00 Install Base\license-cht.txt"
LicenseLangString myLicenseData ${LANG_SIMPCHINESE} "..\00 Install Base\license-chs.txt"
LicenseLangString myLicenseData ${LANG_JAPANESE} "..\00 Install Base\license-jp.txt"

LicenseData $(myLicenseData)

;--------------------------------
; ��װ����
;--------------------------------
Section ""

  SetOutPath $SYSDIR
  ;SetOverwrite off
  ;File "..\00 SYSTEM32\advapi32.dll"

  SetOutPath $INSTDIR
  ;���֮ǰ��װ�������ظ���װʱ��ɾ��֮ǰ��ʵ��
  RMDir /r "D:\Program Files\HaoYin_CheckGenTool\"

  SetOutPath $INSTDIR
  SetOverwrite on
  File /r ".\bin\ParserUI.exe"
  File /r ".\bin\*.dll"
  File /r ".\bin\Music Font.svg"
  File /r ".\bin\script.js"
  File /r ".\bin\skin"

  CreateShortCut "$DESKTOP\CheckGenTool.lnk" "$INSTDIR\ParserUI.exe"

  ClearErrors

SectionEnd
