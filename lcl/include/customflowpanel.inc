{%MainUnit ../extctrls.pp}

{******************************************************************************
                            TCustomFlowPanel
 ******************************************************************************
                             Ondrej Pokorny
 ******************************************************************************
  This file is part of the Lazarus Component Library (LCL)

  See the file COPYING.modifiedLGPL.txt, included in this distribution,
  for details about the license.
 ******************************************************************************
}
{

  Delphi compatibility:

  - TFlowPanel is compatible with Delphi implementation
}

{ TFlowPanelControlList }

constructor TFlowPanelControlList.Create(AOwner: TPersistent);
begin
  Assert(AOwner is TCustomFlowPanel);
  inherited Create(AOwner, TFlowPanelControl);
end;

function TFlowPanelControlList.Add: TFlowPanelControl;
begin
  Result := TFlowPanelControl(inherited Add);
end;

procedure TFlowPanelControlList.AddControl(AControl: TControl;
  AIndex: Integer);
var
  I: Integer;
  Item: TFlowPanelControl;
begin
  if IndexOf(AControl) >= 0 then
    Exit;

  if AIndex = -1 then
    for I := 0 to Count-1 do
      if not Assigned(Items[I].Control) then
      begin
        AIndex := I;
        break;
      end;

  if AIndex = -1 then
    Item := Add
  else
    Item := Items[AIndex];

  Item.FControl := AControl;
end;

function TFlowPanelControlList.FPOwner: TCustomFlowPanel;
begin
  Result := TCustomFlowPanel(GetOwner);
end;

function TFlowPanelControlList.GetItem(Index: Integer
  ): TFlowPanelControl;
begin
  Result := TFlowPanelControl(inherited GetItem(Index));
end;

function TFlowPanelControlList.IndexOf(AControl: TControl): Integer;
begin
  for Result := 0 to Count - 1 do
    if Items[Result].Control = AControl then
      Exit;
  Result := -1;
end;

procedure TFlowPanelControlList.RemoveControl(AControl: TControl);
var
  I: Integer;
begin
  for I := Count - 1 downto 0 do
    if Items[I].Control = AControl then
    begin
      Items[I].FControl := nil;
      Delete(I);
      Exit;
    end;
end;

procedure TFlowPanelControlList.SetItem(Index: Integer;
  const AItem: TFlowPanelControl);
begin
  inherited SetItem(Index, AItem);
end;

{ TFlowPanelControl }

procedure TFlowPanelControl.AssignTo(Dest: TPersistent);
var
  xDest: TFlowPanelControl;
begin
  if Dest is TFlowPanelControl then
  begin
    xDest := TFlowPanelControl(Dest);
    xDest.FWrapAfter := Self.FWrapAfter;
  end else
    inherited AssignTo(Dest);
end;

function TFlowPanelControl.FPCollection: TFlowPanelControlList;
begin
  Result := Collection as TFlowPanelControlList;
end;

function TFlowPanelControl.FPOwner: TCustomFlowPanel;
begin
  Result := FPCollection.FPOwner;
end;

procedure TFlowPanelControl.SetControl(const aControl: TControl);
begin
  if FControl = aControl then Exit;
  Assert(FControl = nil);
  FControl := aControl;
end;

procedure TFlowPanelControl.SetIndex(Value: Integer);
begin
  inherited SetIndex(Value);

  if FPOwner.ComponentState * [csLoading, csUpdating,  csDestroying] = [] then
    FPOwner.ReAlign;
end;

procedure TFlowPanelControl.SetWrapAfter(const AWrapAfter: TWrapAfter);
begin
  if FWrapAfter = AWrapAfter then exit;
  FWrapAfter := AWrapAfter;

  if FPOwner.ComponentState * [csLoading, csUpdating,  csDestroying] = [] then
    FPOwner.ReAlign;
end;

{ TCustomFlowPanel }

constructor TCustomFlowPanel.Create(AOwner: TComponent);
begin
  inherited Create(AOwner);

  FControlList := TFlowPanelControlList.Create(Self);
  FAutoWrap := True;
end;

procedure TCustomFlowPanel.AlignControls(AControl: TControl;
  var RemainingClientRect: TRect);
const
  XIncDir:     array[TFlowStyle] of Integer = (1, -1,  1, -1,  1,  1, -1, -1);
  YIncDir:     array[TFlowStyle] of Integer = (1,  1, -1, -1,  1, -1,  1, -1);
  YDeltaConst: array[TFlowStyle] of Integer = (0,  0, -1, -1,  0,  0,  0,  0);
  XDeltaConst: array[TFlowStyle] of Integer = (0,  0,  0,  0,  0,  0, -1, -1);
var
  I, L: Integer;
  MaxHeight, MaxWidth: Integer;
  YDelta, XDelta: Integer;
  Position: TPoint;
  Size, GroupSize: TSize;
  Control: TControl;
  ConBS: TControlBorderSpacing;
  ForceWrap, ForbidWrap: Boolean;
begin
  DisableAlign;
  try
  if ControlCount > 0 then
  begin
    MaxHeight := 0;
    MaxWidth := 0;
    AdjustClientRect(RemainingClientRect);
    case FFlowStyle of
      fsLeftRightTopBottom,
      fsTopBottomLeftRight:
        Position := RemainingClientRect.TopLeft;
      fsRightLeftTopBottom,
      fsTopBottomRightLeft:
        Position := Point(RemainingClientRect.Right, RemainingClientRect.Top);
      fsLeftRightBottomTop,
      fsBottomTopLeftRight:
        Position := Point(RemainingClientRect.Left, RemainingClientRect.Bottom);
      fsRightLeftBottomTop,
      fsBottomTopRightLeft:
        Position := RemainingClientRect.BottomRight;
    end;

    for I := 0 to FControlList.Count - 1 do
    begin
      Control := FControlList[I].Control;
      ConBS := Control.BorderSpacing;
      if not Control.Visible and not (csDesigning in ComponentState) then
        continue;

      Size.cx := ConBS.ControlWidth;
      Size.cy := ConBS.ControlHeight;
      GroupSize := Size;

      ForceWrap := (I > 0) and (FControlList[I-1].WrapAfter = waForce);
      ForbidWrap := (I > 0) and (FControlList[I-1].WrapAfter = waForbid);
      if not ForceWrap
      and ((I = 0) or not(FControlList[I-1].WrapAfter in [waAvoid, waForbid])) then
      begin
        for L := I to FControlList.Count-2 do
        begin
          if FControlList[L].WrapAfter in [waAvoid, waForbid] then
          begin
            case FFlowStyle of
              fsLeftRightTopBottom, fsLeftRightBottomTop, fsRightLeftTopBottom, fsRightLeftBottomTop:
                Inc(GroupSize.cx, FControlList[L+1].Control.BorderSpacing.ControlWidth);
              fsTopBottomLeftRight, fsTopBottomRightLeft, fsBottomTopLeftRight, fsBottomTopRightLeft:
                Inc(GroupSize.cy, FControlList[L+1].Control.BorderSpacing.ControlHeight);
            end;
          end else
            break;
        end;
      end;
      case FFlowStyle of
        fsLeftRightTopBottom,
        fsLeftRightBottomTop:
          if (MaxHeight > 0) and FAutoWrap and not ForbidWrap
          and (ForceWrap or (Position.X + GroupSize.cx >= RemainingClientRect.Right)) then
          begin
            Inc(Position.Y, MaxHeight * YIncDir[FFlowStyle]);
            MaxHeight := 0;
            Position.X := RemainingClientRect.Left;
          end;
        fsRightLeftTopBottom,
        fsRightLeftBottomTop:
          begin
            Dec(Position.X, GroupSize.cx);
            if (MaxHeight > 0) and FAutoWrap and not ForbidWrap
            and (ForceWrap or (Position.X <= 0)) then
            begin
              Inc(Position.Y, MaxHeight * YIncDir[FFlowStyle]);
              MaxHeight := 0;
              Position.X := RemainingClientRect.Right - Size.cx;
            end;
          end;
        fsTopBottomLeftRight,
        fsTopBottomRightLeft:
          if (MaxWidth > 0) and FAutoWrap and not ForbidWrap
          and (ForceWrap or (Position.Y + GroupSize.cy >= RemainingClientRect.Bottom)) then
          begin
            Inc(Position.X, MaxWidth * XIncDir[FFlowStyle]);
            MaxWidth := 0;
            Position.Y := RemainingClientRect.Top;
          end;
        fsBottomTopLeftRight,
        fsBottomTopRightLeft:
          begin
            Dec(Position.Y, GroupSize.cy);
            if (MaxWidth > 0) and FAutoWrap and not ForbidWrap
            and (ForceWrap or (Position.Y <= 0)) then
            begin
              Inc(Position.X, MaxWidth * XIncDir[FFlowStyle]);
              MaxWidth := 0;
              Position.Y := RemainingClientRect.Bottom - Size.cy;
            end;
          end;
      end;
      YDelta := YDeltaConst[FFlowStyle] * Size.cy;
      XDelta := XDeltaConst[FFlowStyle] * Size.cx;
      if Size.cy > MaxHeight then
        MaxHeight := Size.cy;
      if Size.cx > MaxWidth then
        MaxWidth := Size.cx;
      Control.SetBounds(
        Position.X + ConBS.Left + ConBS.Around + XDelta,
        Position.Y + ConBS.Top + ConBS.Around + YDelta,
        Size.cx - (ConBS.Left + ConBS.Right + ConBS.Around*2),
        Size.cy - (ConBS.Top + ConBS.Bottom + ConBS.Around*2));
      if FFlowStyle in [fsLeftRightTopBottom, fsLeftRightBottomTop] then
        Inc(Position.X, Size.cx * XIncDir[FFlowStyle])
      else if FFlowStyle in [fsTopBottomLeftRight, fsTopBottomRightLeft] then
        Inc(Position.Y, Size.cy + YIncDir[FFlowStyle]);
    end;
  end;
  finally
    EnableAlign;
  end;
end;

procedure TCustomFlowPanel.CalculatePreferredSize(var PreferredWidth,
  PreferredHeight: integer; WithThemeSpace: Boolean);
var
  xControl: TControl;
  xTestRect, xClientRect: TRect;
  I: Integer;
begin
  inherited CalculatePreferredSize(PreferredWidth, PreferredHeight,
    WithThemeSpace);

  if FControlList.Count > 0 then
  begin
    xTestRect := Rect(0, 0, 100, 100);
    xClientRect := xTestRect;
    AdjustClientRect(xClientRect);
    for I := 0 to ControlCount-1 do
    begin
      xControl := FControlList.Items[I].Control;
      if FFlowStyle in [fsLeftRightTopBottom, fsRightLeftTopBottom, fsLeftRightBottomTop, fsRightLeftBottomTop] then
        PreferredHeight := Max(PreferredHeight,
          xControl.BoundsRect.Bottom+xControl.BorderSpacing.Around+xControl.BorderSpacing.Bottom +
          xTestRect.Bottom-xClientRect.Bottom-xTestRect.Top+xClientRect.Top)
      else
        PreferredWidth := Max(PreferredWidth,
          xControl.BoundsRect.Right+xControl.BorderSpacing.Around+xControl.BorderSpacing.Right +
          xTestRect.Right-xClientRect.Right-xTestRect.Left+xClientRect.Left);
    end;
  end;
end;

procedure TCustomFlowPanel.CMControlChange(var Message: TCMControlChange);
begin
  //inherited CMControlChange(Message); uncomment if CMControlChange should appear in parent classes

  if (csLoading in ComponentState) then
    Exit;

  if Message.Inserting and (Message.Control.Parent = Self) then
  begin
    DisableAlign;
    try
      Message.Control.Anchors := [];
      Message.Control.Align := alNone;
      FControlList.AddControl(Message.Control);
      ReAlign;
    finally
      EnableAlign;
    end;
  end else
    FControlList.RemoveControl(Message.Control);
end;

destructor TCustomFlowPanel.Destroy;
begin
  inherited Destroy;
  FControlList.Free;
end;

function TCustomFlowPanel.GetControlIndex(AControl: TControl): Integer;
begin
  Result := FControlList.IndexOf(AControl);
end;

procedure TCustomFlowPanel.SetAutoWrap(const AAutoWrap: Boolean);
begin
  if FAutoWrap = AAutoWrap then Exit;
  FAutoWrap := AAutoWrap;

  ReAlign;
end;

procedure TCustomFlowPanel.SetControlIndex(AControl: TControl; Index: Integer);
var
  CurIndex: Integer;
begin
  CurIndex := GetControlIndex(AControl);
  if (CurIndex > -1) and (CurIndex <> Index) and (Index < FControlList.Count) then
  begin
    FControlList.Items[CurIndex].Index := Index;
    Realign;
  end;
end;

procedure TCustomFlowPanel.SetControlList(
  const AControlList: TFlowPanelControlList);
begin
  FControlList.Assign(AControlList);
end;

procedure TCustomFlowPanel.SetFlowStyle(const AFlowStyle: TFlowStyle);
begin
  if FFlowStyle = AFlowStyle then Exit;
  FFlowStyle := AFlowStyle;

  ReAlign;
end;


// included by extctrls.pp
