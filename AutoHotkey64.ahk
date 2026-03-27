EXE := "IMEstate.exe" ;状态检测器

ToIME() { ;打开/关闭IME
  Send "#{Space}"
}
ToHLF() { ;切换半/全角标点
  Send "{Ctrl Down}.{Ctrl Up}"
}
ToCHN() { ;切换中/英输入状态
  Send("{Ctrl Down}{Space}{Ctrl Up}")
}
ToNRW() { ;切换窄/宽英文
  Send("{Shift Down}{Space}{Shift Up}")
}

WaitIME() { ;等待IME打开
  Loop 10 { ;最多100ms
    if (RunWait(EXE) > 0)
      return 1
    Sleep(10)
  }
  return -1
}
WaitCHN() { ;等待中文输入状态
  Loop 10 { ;最多100ms
    if (2 == Mod(RunWait(EXE) // 100, 10))
      return 1
    Sleep(10)
  }
  return -1
}

CapsLock:: { ;关闭/打开中文输入, 同时换掉默认的全角标点
  old := RunWait(EXE)
  ToIME()
  if (old > 0) ;关闭IME, 直接结束 
    Exit
  if (WaitIME() < 0) ;打开IME失败
    Exit

  new := RunWait(EXE)
  if (2 == Mod(new//10, 10)) ;全角英文
    ToNRW()

  if (1 == Mod(new//100, 10)) { ;英文IME模式
    ToCHN()
    if (WaitCHN() < 0) ;换到中文IME失败
      Exit
  }

  if (2 == Mod(RunWait(EXE)//1, 10)) ;全角标点
    ToHLF()
}

;(Alt),\.;输入全角标点
!,::，
!;::；
!\::、
!.::。
