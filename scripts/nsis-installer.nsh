!macro preInit
  SetRegView 64
  ReadRegStr $0 HKCU "${INSTALL_REGISTRY_KEY}" InstallLocation
  ${If} $0 == ""
    StrCpy $0 "$LOCALAPPDATA\Programs\${PRODUCT_NAME}"
    WriteRegStr HKCU "${INSTALL_REGISTRY_KEY}" InstallLocation "$0"
  ${EndIf}
!macroend
