{%MainUnit lazutf8.pas}

var
  //Function prototypes
  _ParamStrUtf8: Function(Param: Integer): string;

var
  ArgsW: Array of WideString;
  ArgsWCount: Integer; // length(ArgsW)+1
  {$IF (FPC_FULLVERSION >= 30000) AND NOT DEFINED(DisableUTF8RTL)}
  ArgsUTF8: Array of String; // the ArgsW array as UTF8
  OldArgV: PPChar = nil;
  {$IFEND}

//************ START "Stubs" that just call Ansi or Wide implementation

function ParamStrUTF8(Param: Integer): string;
begin
  Result := _ParamStrUtf8(Param);
end;

//************ END "Stubs" that just call Ansi or Wide implementation


//*************** START Non WideString implementations
{$ifndef wince}
function ParamStrUtf8Ansi(Param: Integer): String;
begin
  Result:=SysToUTF8(ObjPas.ParamStr(Param));
end;
{$endif wince}

//*************** END Non WideString impementations




//*************** START WideString impementations

//fpc 2.6.0 does not have StrLen for WideChar.
//Remove this when we have 2.6.4 or higher
{$if FPC_FULLVERSION < 20602}
function StrLen(PW: PWideChar): SizeInt; overload;
var
  i: SizeInt;
begin
  i:=0;
  if assigned(PW) then
    while (PW[i] <> #0) do inc(i);
  Result := i;
end;
{$endif}


procedure SetupCommandlineParametersWide;
var
  ArgLen, Start, CmdLen, i, j: SizeInt;
  Quote   : Boolean;
  Buf: array[0..259] of WChar;  // need MAX_PATH bytes, not 256!
  PCmdLineW: PWideChar;
  CmdLineW: WideString;

  procedure AllocArg(Idx, Len:longint);
  begin
    if (Idx >= ArgsWCount) then
      SetLength(ArgsW, Idx + 1);
    SetLength(ArgsW[Idx], Len);
  end;

begin
  { create commandline, it starts with the executed filename which is argv[0] }
  { Win32 passes the command NOT via the args, but via getmodulefilename}
  ArgsWCount := 0;
  ArgLen := GetModuleFileNameW(0, @buf[0], sizeof(buf));

  //writeln('ArgLen = ',Arglen);

  buf[ArgLen] := #0; // be safe, no terminating 0 on XP
  allocarg(0,arglen);
  move(buf[0],ArgsW[0][1],arglen * SizeOf(WChar));

  //writeln('ArgsW[0] = ',ArgsW[0]);

  PCmdLineW := nil;
  { Setup cmdline variable }
  PCmdLineW := GetCommandLineW;
  CmdLen := StrLen(PCmdLineW);

  //writeln('StrLen(PCmdLineW) = ',CmdLen);

  SetLength(CmdLineW, CmdLen);
  Move(PCmdLineW^, CmdLineW[1], CmdLen * SizeOf(WChar));


  //debugln(CmdLineW);
  //for i := 1 to CmdLen do DbgOut(DbgS(i mod 10)); debugln;

  i := 1;
  while (i <= CmdLen) do
  begin
    //debugln('Next');
    //DbgOut('i=',DbgS(i),' CmdLineW[',DbgS(i),']=');if i<=CmdLen then debugln(CmdLineW[i]) else debugln('#0');
    //skip leading spaces
    while (i <= CmdLen) and (CmdLineW[i] <= #32) do Inc(i);
    //DbgOut('After skipping spaces: i=',DbgS(i),' CmdLineW[',DbgS(i),']=');if i<=CmdLen then debugln(CmdLineW[i]) else debugln('#0');
    if (i > CmdLen) then Break;
    Quote := False;
    Start := i;
    ArgLen := 0;
    while (i <= CmdLen) do
    begin //find next commandline parameter
      case CmdLineW[i] of
        #1..#32:
        begin
          if Quote then
          begin
            //debugln('i=',DbgS(i),': Space in Quote');
            Inc(ArgLen)
          end
          else
          begin
            //debugln('i=',DbgS(i),': Space in NOT Quote');
            Break;
          end;
        end;
        '"':
        begin
          if (i < CmdLen) and (CmdLineW[i+1] <> '"') then
          begin
            //debugln('i=',DbgS(i),': Quote := not Quote');
            Quote := not Quote
          end
          else
          begin
            //debugln('i=',DbgS(i),': Skip Quote');
            Inc(i);
          end;
        end;
        else Inc(ArgLen);
      end;//case
      Inc(i);
    end; //find next commandline parameter

    //debugln('ArgWCount=',DbgS(ArgsWCount),' Start=',DbgS(start),' ArgLen=',DbgS(arglen),' i=',DbgS(i));

    //we already have (a better) ArgW[0]
    if (ArgsWCount > 0) then
    begin //Process commandline parameter
      AllocArg(ArgsWCount, ArgLen);
      Quote := False;
      i := Start;
      j := 1;
      while (i <= CmdLen) do
      begin
        case CmdLineW[i] of
          #1..#32:
          begin
            if Quote then
            begin
              //if j > ArgLen then debugln('Error whitespace: j > ArgLen: j=',DbgS(j),' ArgLen=',DbgS(arglen));
              ArgsW[ArgsWCount][j] := CmdLineW[i];
              Inc(j);
            end
            else
              Break;
          end;
          '"':
          begin
            if (i < CmdLen) and (CmdLineW[i+1] <> '"') then
              Quote := not Quote
            else
              Inc(i);
          end;
          else
          begin
            //if j > ArgLen then debugln('Error Quote: j > ArgLen: j=',DbgS(j),' ArgLen=',DbgS(arglen));
            ArgsW[ArgsWCount][j] := CmdLineW[i];
            Inc(j);
          end;
        end;
        Inc(i);
      end;

      //debugln('ArgsW[',DbgS(ArgsWCount),'] = ',ArgsW[ArgsWCount]);
    end; // Process commandline parameter
    Inc(ArgsWCount);

  end;
  Dec(ArgsWCount);

  {$IF (FPC_FULLVERSION >= 30000) AND NOT DEFINED(DisableUTF8RTL)}
  if DefaultSystemCodePage=CP_UTF8 then begin
    SetLength(ArgsUTF8,length(ArgsW));
    OldArgV:=argv;
    GetMem(argv,SizeOf(Pointer)*length(ArgsW));
    for i:=0 to length(ArgsW)-1 do
    begin
      ArgsUTF8[i]:=ArgsW{%H-}[i];
      argv[i]:=PChar(ArgsUTF8[i]);
    end;
  end;
  {$IFEND}
end;

function ParamStrUtf8Wide(Param: Integer): String;
begin
  if ArgsWCount <> ParamCount then
  begin
    //DebugLn('Error: ParamCount <> ArgsWCount!');
    Result := SysToUtf8(ObjPas.ParamStr(Param));
  end
  else
  begin
    if (Param <= ArgsWCount) then
      Result := Utf8Encode(ArgsW[Param])
    else
      Result := '';
  end;
end;

//*************** END WideString impementations



function ConsoleToUTF8(const s: string): string;// converts UTF8 string to console encoding (used by Write, WriteLn)
{$ifNdef WinCE}
var
  Dst: PChar;
{$endif}
begin
  {$ifdef WinCE}
  Result := SysToUTF8(s);
  {$else}
  Dst := AllocMem((Length(s) + 1) * SizeOf(Char));
  if OemToChar(PChar(s), Dst) then
    Result := StrPas(Dst)
  else
    Result := s;
  FreeMem(Dst);
  Result := SysToUTF8(Result);
  {$endif}
end;

function UTF8ToConsole(const s: string): string;
{$ifNdef WinCE}
var
  Dst: PChar;
{$endif}
begin
  {$ifdef WinCE}
  Result := UTF8ToSys(s);
  {$else}
  Result := UTF8ToSys(s);
  Dst := AllocMem((Length(Result) + 1) * SizeOf(Char));
  if CharToOEM(PChar(Result), Dst) then
    Result := StrPas(Dst);
  FreeMem(Dst);
  {$endif}
end;

{$IFDEF MSWindows}
// for all Windows supporting 8bit codepages (e.g. not WinCE)
function WinCPToUTF8(const s: string): string;
// result has codepage CP_ACP
var
  UTF16WordCnt: SizeInt;
  UTF16Str: UnicodeString;
begin
  Result:=s;
  if IsASCII(Result) then begin
    {$ifdef FPC_HAS_CPSTRING}
    // prevent codepage conversion magic
    SetCodePage(RawByteString(Result), CP_ACP, False);
    {$endif}
    exit;
  end;
  UTF16WordCnt:=MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, Pointer(s), length(s), nil, 0);
  // this will null-terminate
  if UTF16WordCnt>0 then
  begin
    setlength(UTF16Str, UTF16WordCnt);
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, Pointer(s), length(s), @UTF16Str[1], UTF16WordCnt);
    Result:=UTF8Encode(UTF16Str);
    {$ifdef FPC_HAS_CPSTRING}
    // prevent codepage conversion magic
    SetCodePage(RawByteString(Result), CP_ACP, False);
    {$endif}
  end;
end;

function UTF8ToWinCP(const s: string): string;
// result has codepage CP_ACP
var
  src: UnicodeString;
  len: LongInt;
begin
  Result:=s;
  if IsASCII(Result) then begin
    {$ifdef FPC_HAS_CPSTRING}
    // prevent codepage conversion magic
    SetCodePage(RawByteString(Result), CP_ACP, False);
    {$endif}
    exit;
  end;
  src:=UTF8Decode(s);
  if src='' then
    exit;
  len:=WideCharToMultiByte(CP_ACP,0,PUnicodeChar(src),length(src),nil,0,nil,nil);
  SetLength(Result,len);
  if len>0 then begin
    WideCharToMultiByte(CP_ACP,0,PUnicodeChar(src),length(src),@Result[1],length(Result),nil,nil);
    {$ifdef FPC_HAS_CPSTRING}
    // prevent codepage conversion magic
    SetCodePage(RawByteString(Result), CP_ACP, False);
    {$endif}
  end;
end;
{$ENDIF}

{$ifdef debugparamstrutf8}
procedure ParamStrUtf8Error;
var
  i: Integer;
begin
  writeln('Error in Windows WideString implementation of ParamStrUtf8');
  writeln('Using SysToUtf8(ParamsStr(Param)) as fallback');
  writeln('ParamCount = ',ParamCount,', ArgsWCount = ',ArgsWCount);
  for i := 0 to ParamCount do writeln('ParamStr(',i,') = "',ParamStr(i),'"');
  writeln;
  for i := 0 to ArgsWCount do writeln('ParamStrUtf8(',i,') = "',ArgsW[i],'"');
end;
{$endif}

{$IF (FPC_FULLVERSION >= 30000) AND NOT DEFINED(DisableUTF8RTL)}
function GetLocaleStr(aLocaleID, aLCType: Longint; const Def: string): String;
var
  L: Integer;
  Buf: array[0..255] of WideChar;
begin
  L := GetLocaleInfoW(aLocaleID, aLCType, Buf, SizeOf(Buf));
  if L > 0 then
  begin
    Result:='';
    widestringmanager.Wide2AnsiMoveProc(PWideChar(@Buf[0]),Result,CP_UTF8,L-1);
  end
  else
    Result := Def;
end;

function GetLocaleCharUTF8(aLocaleID, aLCType: Longint; Def: Char): Char;
var
  Buf: array[0..3] of Char; // sdate allows 4 chars.
begin
  if (GetLocaleInfoA(aLocaleID, aLCType, Buf, sizeof(buf)) > 0)
  and (ord(Buf[0])<128) then
    Result := Buf[0]
  else begin
    Result := Def;
    case Buf[0] of
    #$C2:
      case Buf[1] of
      #$A0: Result:=' '; // non breakable space
      #$B7: Result:='.'; // middle stop
      end;
    #$CB:
      if Buf[1]=#$99 then Result:=''''; // dot above, italian handwriting
    #$D9:
      case Buf[1] of
      #$AB: Result:=','; // arabic decimal separator, persian thousand separator
      #$AC: Result:=''''; // arabic thousand separator
      end;
    #$E2:
      case Buf[1] of
      #$80:
        case Buf[2] of
        #$82, // long space
        #$83, // long space
        #$89, // thin space
        #$AF: // narrow non breakable space
          Result := ' ';
        #$94: Result := '-'; // persian decimal mark
        end;
      #$8E: if Buf[2]=#$96 then Result := ''''; // codepoint 9110 decimal separator
      end;
    end;
  end;
end;

procedure GetFormatSettingsUTF8(LCID: Integer; var aFormatSettings: TFormatSettings);
var
  HF  : Shortstring;
  LID : Windows.LCID;
  I,Day : longint;
begin
  LID := LCID;
  with aFormatSettings do
  begin
    { Date stuff }
    for I := 1 to 12 do
      begin
      ShortMonthNames[I]:=GetLocaleStr(LID,LOCALE_SABBREVMONTHNAME1+I-1,ShortMonthNames[i]);
      LongMonthNames[I]:=GetLocaleStr(LID,LOCALE_SMONTHNAME1+I-1,LongMonthNames[i]);
      end;
    for I := 1 to 7 do
      begin
      Day := (I + 5) mod 7;
      ShortDayNames[I]:=GetLocaleStr(LID,LOCALE_SABBREVDAYNAME1+Day,ShortDayNames[i]);
      LongDayNames[I]:=GetLocaleStr(LID,LOCALE_SDAYNAME1+Day,LongDayNames[i]);
      end;
    DateSeparator := GetLocaleCharUTF8(LID, LOCALE_SDATE, '/');
    ShortDateFormat := GetLocaleStr(LID, LOCALE_SSHORTDATE, 'm/d/yy');
    LongDateFormat := GetLocaleStr(LID, LOCALE_SLONGDATE, 'mmmm d, yyyy');
    { Time stuff }
    TimeSeparator := GetLocaleCharUTF8(LID, LOCALE_STIME, ':');
    TimeAMString := GetLocaleStr(LID, LOCALE_S1159, 'AM');
    TimePMString := GetLocaleStr(LID, LOCALE_S2359, 'PM');
    if StrToIntDef(GetLocaleStr(LID, LOCALE_ITLZERO, '0'), 0) = 0 then
      HF:='h'
    else
      HF:='hh';
    // No support for 12 hour stuff at the moment...
    ShortTimeFormat := HF+':nn';
    LongTimeFormat := HF + ':nn:ss';
    { Currency stuff }
    CurrencyString:=GetLocaleStr(LID, LOCALE_SCURRENCY, '');
    CurrencyFormat:=StrToIntDef(GetLocaleStr(LID, LOCALE_ICURRENCY, '0'), 0);
    NegCurrFormat:=StrToIntDef(GetLocaleStr(LID, LOCALE_INEGCURR, '0'), 0);
    { Number stuff }
    ThousandSeparator:=GetLocaleCharUTF8(LID, LOCALE_STHOUSAND, ',');
    DecimalSeparator:=GetLocaleCharUTF8(LID, LOCALE_SDECIMAL, '.');
    CurrencyDecimals:=StrToIntDef(GetLocaleStr(LID, LOCALE_ICURRDIGITS, '0'), 0);
    ListSeparator := GetLocaleCharUTF8(LID, LOCALE_SLIST, ',');
  end;
end;

function UTF8StrCompAnsiString(S1, S2: PChar): PtrInt;
begin
  Result:=UTF8CompareStrP(S1,S2);
end;

function UTF8StrICompAnsiString(S1, S2: PChar): PtrInt;
var
  U1, U2: String;
begin
  U1:=StrPas(S1);
  U2:=StrPas(S2);
  Result:=UTF8CompareText(U1,U2);
end;

function UTF8StrLCompAnsiString(S1, S2: PChar; Count: PtrUInt): PtrInt;
begin
  Result:=UTF8CompareStr(S1,Count,S2,Count);
end;

function UTF8StrLICompAnsiString(S1, S2: PChar; Count: PtrUInt): PtrInt;
var
  U1, U2: String;
begin
  if Count>0 then begin
    SetLength(U1,Count);
    Move(S1^,PByte(U1)^,Count);
    SetLength(U2,Count);
    Move(S2^,PByte(U2)^,Count);
    Result:=UTF8CompareText(U1,U2);
  end else
    Result:=0;
end;
{$IFEND}

procedure InitLazUtf8;
begin
  {$ifndef WinCE}
  if Win32MajorVersion <= 4 then
  begin
    _ParamStrUtf8 := @ParamStrUtf8Ansi;
  end
  else
  {$endif}
  begin
    try
      ArgsWCount := -1;
      _ParamStrUtf8 := @ParamStrUtf8Wide;
      SetupCommandlineParametersWide;
      {$ifdef debugparamstrutf8}
      if ParamCount <> ArgsWCount then ParamStrUtf8Error;
      {$endif}
    Except
      begin
        ArgsWCount := -1;
        {$ifdef debugparamstrutf8}
        ParamStrUtf8Error;
        {$endif}
      end;
    end;
  end;
  {$IF (FPC_FULLVERSION >= 30000) AND NOT DEFINED(DisableUTF8RTL)}
  GetFormatSettingsUTF8(GetThreadLocale,FormatSettings);
  widestringmanager.UpperAnsiStringProc:=@UTF8UpperString;
  widestringmanager.LowerAnsiStringProc:=@UTF8LowerString;
  widestringmanager.CompareStrAnsiStringProc:=@UTF8CompareStr;
  widestringmanager.CompareTextAnsiStringProc:=@UTF8CompareText;
  widestringmanager.StrCompAnsiStringProc:=@UTF8StrCompAnsiString;
  widestringmanager.StrICompAnsiStringProc:=@UTF8StrICompAnsiString;
  widestringmanager.StrLCompAnsiStringProc:=@UTF8StrLCompAnsiString;
  widestringmanager.StrLICompAnsiStringProc:=@UTF8StrLICompAnsiString;
  // Does anyone need these two?
  //widestringmanager.StrLowerAnsiStringProc;
  //widestringmanager.StrUpperAnsiStringProc;
  {$IFEND}
end;

procedure FinalizeLazUTF8;
{$IF (FPC_FULLVERSION >= 30000) AND NOT DEFINED(DisableUTF8RTL)}
var
  p: PPChar;
{$IFEND}
begin
  {$IF (FPC_FULLVERSION >= 30000) AND NOT DEFINED(DisableUTF8RTL)}
  // restore argv and free memory
  if OldArgV<>nil then
  begin
    p:=argv;
    argv:=OldArgV;
    Freemem(p);
  end;
  {$IFEND}
end;
