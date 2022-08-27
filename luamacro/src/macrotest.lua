-- encoding: utf-8
-- Started: 2012-08-20.

local function assert_eq(a,b)   assert(a == b) end
local function assert_neq(a,b)  assert(a ~= b) end
local function assert_num(v)    assert(type(v)=="number") end
local function assert_str(v)    assert(type(v)=="string") end
local function assert_tbl(v)    assert(type(v)=="table") end
local function assert_bool(v)   assert(type(v)=="boolean") end
local function assert_func(v)   assert(type(v)=="function") end
local function assert_nil(v)    assert(v==nil) end
local function assert_false(v)  assert(v==false) end
local function assert_true(v)   assert(v==true) end
local function assert_falsy(v)  assert(not v == true) end
local function assert_truthy(v) assert(not v == false) end

local function assert_range(val, low, high)
  if low then assert(val >= low) end
  if high then assert(val <= high) end
end

local MT = {} -- "macrotest", this module
local F = far.Flags
local luamacroId=0x4EBBEFC8

local function pack (...)
  return { n=select("#",...), ... }
end

local function IsNumOrInt(v)
  return type(v)=="number" or bit64.type(v)
end

local TmpFileName = "/tmp/tmp.tmp"

local function WriteTmpFile(...)
  local fp = assert(io.open(TmpFileName,"w"))
  fp:write(...)
  fp:close()
end

local function DeleteTmpFile()
  win.DeleteFile(TmpFileName)
end

local function TestArea (area, msg)
  assert(Area[area]==true and Area.Current==area, msg or "assertion failed!")
end

function MT.test_areas()
  Keys "AltIns"              TestArea "Other"      Keys "Esc"
  Keys "F12 0"               TestArea "Shell"
  Keys "ShiftF4 CtrlY Enter" TestArea "Editor"     Keys "Esc"
  Keys "F7"                  TestArea "Dialog"     Keys "Esc"
  Keys "Alt/"                TestArea "Search"     Keys "Esc"
  Keys "AltF1"               TestArea "Disks"      Keys "Esc"
  Keys "AltF2"               TestArea "Disks"      Keys "Esc"
  Keys "F9"                  TestArea "MainMenu"   Keys "Esc"
  Keys "F9 Enter"            TestArea "MainMenu"   Keys "Esc Esc"
  Keys "F12"                 TestArea "Menu"       Keys "Esc"
  Keys "F1"                  TestArea "Help"       Keys "Esc"
  Keys "CtrlL Tab"           TestArea "Info"       Keys "Tab CtrlL"
  Keys "CtrlQ Tab"           TestArea "QView"      Keys "Tab CtrlQ"
  Keys "CtrlT Tab"           TestArea "Tree"       Keys "Tab CtrlT"
  Keys "AltF10"              TestArea "FindFolder" Keys "Esc"
  Keys "F2"                  TestArea "UserMenu"   Keys "Esc"

  assert(Area.Current              =="Shell")
  assert(Area.Other                ==false)
  assert(Area.Shell                ==true)
  assert(Area.Viewer               ==false)
  assert(Area.Editor               ==false)
  assert(Area.Dialog               ==false)
  assert(Area.Search               ==false)
  assert(Area.Disks                ==false)
  assert(Area.MainMenu             ==false)
  assert(Area.Menu                 ==false)
  assert(Area.Help                 ==false)
  assert(Area.Info                 ==false)
  assert(Area.QView                ==false)
  assert(Area.Tree                 ==false)
  assert(Area.FindFolder           ==false)
  assert(Area.UserMenu             ==false)
  assert(Area.AutoCompletion       ==false)
end

local function test_mf_akey()
  assert(akey == mf.akey)
  local k0,k1 = akey(0),akey(1)
  assert(k0==0x0501007B and k1=="CtrlShiftF12" or
         k0==0x1401007B and k1=="RCtrlShiftF12")
  -- (the 2nd parameter is tested in function test_mf_eval).
end

local function test_bit64()
  for _,name in ipairs{"band","bnot","bor","bxor","lshift","rshift"} do
    assert(_G[name] == bit64[name])
    assert(type(bit64[name]) == "function")
  end

  local a,b,c = 0xFF,0xFE,0xFD
  assert(band(a,b,c,a,b,c) == 0xFC)
  a,b,c = bit64.new(0xFF),bit64.new(0xFE),bit64.new(0xFD)
  assert(band(a,b,c,a,b,c) == 0xFC)

  a,b = bit64.new("0xFFFF0000FFFF0000"),bit64.new("0x0000FFFF0000FFFF")
  assert(band(a,b) == 0)
  assert(bor(a,b) == -1)
  assert(a+b == -1)

  a,b,c = 1,2,4
  assert(bor(a,b,c,a,b,c) == 7)

  for k=-3,3 do assert(bnot(k) == -1-k) end
  assert(bnot(bit64.new(5)) == -6)

  assert(bxor(0x01,0xF0,0xAA) == 0x5B)
  assert(lshift(0xF731,4) == 0xF7310)
  assert(rshift(0xF7310,4) == 0xF731)

  local v = bit64.new(5)
  assert(v+2==7  and 2+v==7)
  assert(v-2==3  and 2-v==-3)
  assert(v*2==10 and 2*v==10)
  assert(v/2==2  and 2/v==0)
  assert(v%2==1  and 2%v==2)
  assert(v+v==10 and v-v==0 and v*v==25 and v/v==1 and v%v==0)

  local w = lshift(1,63)
  assert(w == bit64.new("0x8000".."0000".."0000".."0000"))
  assert(rshift(w,63)==1)
  assert(rshift(w,64)==0)
  assert(bit64.arshift(w,62)==-2)
  assert(bit64.arshift(w,63)==-1)
  assert(bit64.arshift(w,64)==-1)
end

local function test_mf_eval()
  assert(eval==mf.eval)

  -- test arguments validity checking
  assert(eval("") == 0)
  assert(eval("", 0) == 0)
  assert(eval() == -1)
  assert(eval(0) == -1)
  assert(eval(true) == -1)
  assert(eval("", -1) == -1)
  assert(eval("", 5) == -1)
  assert(eval("", true) == -1)
  assert(eval("", 1, true) == -1)
  assert(eval("",1,"javascript")==-1)

  -- test macro-not-found error
  assert(eval("", 2) == -2)

  temp=3
  assert(eval("temp=5+7")==0)
  assert(temp==12)

  temp=3
  assert(eval("temp=5+7",0,"moonscript")==0)
  assert(eval("temp=5+7",1,"lua")==0)
  assert(eval("temp=5+7",3,"lua")=="")
  assert(eval("temp=5+7",1,"moonscript")==0)
  assert(eval("temp=5+7",3,"moonscript")=="")
  assert(temp==3)
  assert(eval("getfenv(1).temp=12",0,"moonscript")==0)
  assert(temp==12)

  assert(eval("5",0,"moonscript")==0)
  assert(eval("5+7",1,"lua")==11)
  assert(eval("5+7",1,"moonscript")==0)
  assert(eval("5 7",1,"moonscript")==11)

  -- test with Mode==2
  local Id = assert(far.MacroAdd(nil,nil,"CtrlA",[[
    local key = akey(1,0)
    assert(key=="CtrlShiftF12" or key=="RCtrlShiftF12")
    assert(akey(1,1)=="CtrlA")
    foobar = (foobar or 0) + 1
    return foobar,false,5,nil,"foo"
  ]]))
  for k=1,3 do
    local ret1,a,b,c,d,e = eval("CtrlA",2)
    assert(ret1==0 and a==k and b==false and c==5 and d==nil and e=="foo")
  end
  assert(far.MacroDelete(Id))
end

local function test_mf_abs()
  assert(mf.abs(1.3)==1.3)
  assert(mf.abs(-1.3)==1.3)
  assert(mf.abs(0)==0)
end

local function test_mf_acall()
  local a,b,c,d = mf.acall(function(p) return 3, nil, p, "foo" end, 77)
  assert(a==3 and b==nil and c==77 and d=="foo")
  assert(true == mf.acall(far.Show))
  Keys"Esc"
end

local function test_mf_asc()
  assert(mf.asc("0")==48)
  assert(mf.asc("Я")==1071)
end

local function test_mf_atoi()
  assert(mf.atoi("0")==0)
  assert(mf.atoi("-10")==-10)
  assert(mf.atoi("0x11")==17)
  assert(mf.atoi("1011",2)==11)
  assert(mf.atoi("123456789123456789")==bit64.new("123456789123456789"))
  assert(mf.atoi("-123456789123456789")==bit64.new("-123456789123456789"))
  assert(mf.atoi("0x1B69B4BACD05F15")==bit64.new("0x1B69B4BACD05F15"))
  assert(mf.atoi("-0x1B69B4BACD05F15")==bit64.new("-0x1B69B4BACD05F15"))
end

local function test_mf_chr()
  assert(mf.chr(48)=="0")
  assert(mf.chr(1071)=="Я")
end

local function test_mf_clip()
  local oldval = far.PasteFromClipboard() -- store

  mf.clip(5,2) -- turn on the internal clipboard
  assert(mf.clip(5,-1)==2)
  assert(mf.clip(5,1)==2) -- turn on the OS clipboard
  assert(mf.clip(5,-1)==1)

  for clipnum=1,2 do
    mf.clip(5,clipnum)
    local str = "foo"..clipnum
    assert(mf.clip(1,str) > 0)
    assert(mf.clip(0) == str)
    assert(mf.clip(2,"bar") > 0)
    assert(mf.clip(0) == str.."bar")
  end

  mf.clip(5,1); mf.clip(1,"foo")
  mf.clip(5,2); mf.clip(1,"bar")
  assert(mf.clip(0) == "bar")
  mf.clip(5,1); assert(mf.clip(0) == "foo")
  mf.clip(5,2); assert(mf.clip(0) == "bar")

  mf.clip(3);   assert(mf.clip(0) == "foo")
  mf.clip(5,1); assert(mf.clip(0) == "foo")

  mf.clip(5,2); mf.clip(1,"bar")
  mf.clip(5,1); assert(mf.clip(0) == "foo")
  mf.clip(4);   assert(mf.clip(0) == "bar")
  mf.clip(5,2); assert(mf.clip(0) == "bar")

  mf.clip(5,1) -- leave the OS clipboard active in the end
  far.CopyToClipboard(oldval or "") -- restore
end

local function test_mf_env()
  mf.env("Foo",1,"Bar")
  assert(mf.env("Foo")=="Bar")
  mf.env("Foo",1,"")
  assert(mf.env("Foo")=="")
end

local function test_mf_fattr()
  DeleteTmpFile()
  assert(mf.fattr(TmpFileName) == -1)
  WriteTmpFile("")
  local attr = mf.fattr(TmpFileName)
  DeleteTmpFile()
  assert(attr >= 0)
end

local function test_mf_fexist()
  WriteTmpFile("")
  assert(mf.fexist(TmpFileName) == true)
  DeleteTmpFile()
  assert(mf.fexist(TmpFileName) == false)
end

local function test_mf_msgbox()
  assert(msgbox == mf.msgbox)
  mf.postmacro(function() Keys("Esc") end)
  assert(0 == msgbox("title","message"))
  mf.postmacro(function() Keys("Enter") end)
  assert(1 == msgbox("title","message"))
end

local function test_mf_prompt()
  assert(prompt == mf.prompt)
  mf.postmacro(function() Keys("a b c Esc") end)
  assert(not prompt())
  mf.postmacro(function() Keys("a b c Enter") end)
  assert("abc" == prompt())
end

local function test_mf_date()
  assert(type(mf.date())=="string")
  assert(type(mf.date("%a"))=="string")
end

local function test_mf_fmatch()
  assert(mf.fmatch("Readme.txt", "*.txt") == 1)
  assert(mf.fmatch("Readme.txt", "Readme.*|*.txt") == 0)
  assert(mf.fmatch("c:\\Readme.txt", "/txt$/i") == 1)
  assert(mf.fmatch("c:\\Readme.txt", "/txt$") == -1)
end

local function test_mf_fsplit()
  local path="C:/Program Files/Far/Far.exe"
  assert(mf.fsplit(path,0x01)=="C:/")
  assert(mf.fsplit(path,0x02)=="/Program Files/Far/")
  assert(mf.fsplit(path,0x04)=="Far")
  assert(mf.fsplit(path,0x08)==".exe")

  assert(mf.fsplit(path,0x03)=="C:/Program Files/Far/")
  assert(mf.fsplit(path,0x0C)=="Far.exe")
  assert(mf.fsplit(path,0x0F)==path)
end

local function test_mf_iif()
  assert(mf.iif(true,  1, 2)==1)
  assert(mf.iif("a",   1, 2)==1)
  assert(mf.iif(100,   1, 2)==1)
  assert(mf.iif(false, 1, 2)==2)
  assert(mf.iif(nil,   1, 2)==2)
  assert(mf.iif(0,     1, 2)==2)
  assert(mf.iif("",    1, 2)==2)
end

local function test_mf_index()
  assert(mf.index("language","gua",0)==3)
  assert(mf.index("language","gua",1)==3)
  assert(mf.index("language","gUA",1)==-1)
  assert(mf.index("language","gUA",0)==3)
end

local function test_mf_int()
  assert(mf.int("2.99")==2)
  assert(mf.int("-2.99")==-2)
  assert(mf.int("0x10")==0)
  assert(mf.int("123456789123456789")==bit64.new("123456789123456789"))
  assert(mf.int("-123456789123456789")==bit64.new("-123456789123456789"))
end

local function test_mf_itoa()
  assert(mf.itoa(100)=="100")
  assert(mf.itoa(100,10)=="100")
  assert(mf.itoa(bit64.new("123456789123456789"))=="123456789123456789")
  assert(mf.itoa(bit64.new("-123456789123456789"))=="-123456789123456789")
  assert(mf.itoa(100,2)=="1100100")
  assert(mf.itoa(100,16)=="64")
  assert(mf.itoa(100,36)=="2s")
end

local function test_mf_key()
  assert(mf.key(0x01000000)=="Ctrl")
  assert(mf.key(0x02000000)=="Alt")
  assert(mf.key(0x04000000)=="Shift")
  assert(mf.key(0x10000000)=="RCtrl")
  assert(mf.key(0x20000000)=="RAlt")

  assert(mf.key(0x0501007B)=="CtrlShiftF12")
  assert(mf.key("CtrlShiftF12")=="CtrlShiftF12")

  assert(mf.key("foobar")=="")
end

-- Separate tests for mf.float and mf.string are locale-dependant, thus they are tested together.
local function test_mf_float_and_string()
  local t = { 0, -0, 2.56e1, -5.37, -2.2e100, 2.2e-100 }
  for _,num in ipairs(t) do
    assert(mf.float(mf.string(num))==num)
  end
end

local function test_mf_lcase()
  assert(mf.lcase("FOo БАр")=="foo бар")
end

local function test_mf_len()
  assert(mf.len("")==0)
  assert(mf.len("FOo БАр")==7)
end

local function test_mf_max()
  assert(mf.max(-2,-5)==-2)
  assert(mf.max(2,5)==5)
end

local function test_mf_min()
  assert(mf.min(-2,-5)==-5)
  assert(mf.min(2,5)==2)
end

local function test_mf_msave()
  local Key = "macrotest"

  -- test supported types, except tables
  local v1, v2, v3, v4, v5, v6 = nil, false, true, -5.67, "foo", bit64.new("0x1234567843218765")
  mf.msave(Key, "name1", v1)
  mf.msave(Key, "name2", v2)
  mf.msave(Key, "name3", v3)
  mf.msave(Key, "name4", v4)
  mf.msave(Key, "name5", v5)
  mf.msave(Key, "name6", v6)
  assert(mf.mload(Key, "name1") == v1)
  assert(mf.mload(Key, "name2") == v2)
  assert(mf.mload(Key, "name3") == v3)
  assert(mf.mload(Key, "name4") == v4)
  assert(mf.mload(Key, "name5") == v5)
  assert(mf.mload(Key, "name6") == v6)
  mf.mdelete(Key, "*")
  assert(mf.mload(Key, "name3")==nil)

  -- test tables
  mf.msave(Key, "name1", { a=5, {b="foo"}, c={d=false} })
  local t=mf.mload(Key, "name1")
  assert(t.a==5 and t[1].b=="foo" and t.c.d==false)
  mf.mdelete(Key, "name1")
  assert(mf.mload(Key, "name1")==nil)

  -- test tables more
  local t1, t2, t3 = {5}, {6}, {}
  t1[2], t1[3], t1[4], t1[5] = t1, t2, t3, t3
  t2[2], t2[3] = t1, t2
  t1[t1], t1[t2] = 66, 77
  t2[t1], t2[t2] = 88, 99
  setmetatable(t3, { __index=t1 })
  mf.msave(Key, "name1", t1)

  local T1 = mf.mload(Key, "name1")
  assert(type(T1)=="table")
  local T2 = T1[3]
  assert(type(T2)=="table")
  local T3 = T1[4]
  assert(type(T3)=="table" and T3==T1[5])
  assert(T1[1]==5 and T1[2]==T1 and T1[3]==T2)
  assert(T2[1]==6 and T2[2]==T1 and T2[3]==T2)
  assert(T1[T1]==66 and T1[T2]==77)
  assert(T2[T1]==88 and T2[T2]==99)
  assert(getmetatable(T3).__index==T1 and T3[1]==5 and rawget(T3,1)==nil)
  mf.mdelete(Key, "*")
  assert(mf.mload(Key, "name1")==nil)
end

local function test_mf_mod()
  assert(mf.mod(11,4) == 3)
  assert(math.fmod(11,4) == 3)
  assert(11 % 4 == 3)

  assert(mf.mod(-1,4) == -1)
  assert(math.fmod(-1,4) == -1)
  assert(-1 % 4 == 3)
end

local function test_mf_replace()
  assert(mf.replace("Foo Бар", "o", "1")=="F11 Бар")
  assert(mf.replace("Foo Бар", "o", "1", 1)=="F1o Бар")
  assert(mf.replace("Foo Бар", "O", "1", 1, 1)=="Foo Бар")
  assert(mf.replace("Foo Бар", "O", "1", 1, 0)=="F1o Бар")
end

local function test_mf_rindex()
  assert(mf.rindex("language","a",0)==5)
  assert(mf.rindex("language","a",1)==5)
  assert(mf.rindex("language","A",1)==-1)
  assert(mf.rindex("language","A",0)==5)
end

local function test_mf_strpad()
  assert(mf.strpad("Foo",10,"*",  2) == '***Foo****')
  assert(mf.strpad("",   10,"-*-",2) == '-*--*--*--')
  assert(mf.strpad("",   10,"-*-")   == '-*--*--*--')
  assert(mf.strpad("Foo",10)         == 'Foo       ')
  assert(mf.strpad("Foo",10,"-")     == 'Foo-------')
  assert(mf.strpad("Foo",10," ",  1) == '       Foo')
  assert(mf.strpad("Foo",10," ",  2) == '   Foo    ')
  assert(mf.strpad("Foo",10,"1234567890",2) == '123Foo1234')
end

local function test_mf_strwrap()
  assert(mf.strwrap("Пример строки, которая будет разбита на несколько строк по ширине в 7 символов.", 7,"\n")==
[[
Пример
строки,
которая
будет
разбита
на
несколь
ко
строк
по
ширине
в 7
символо
в.]])
end

local function test_mf_substr()
  assert(mf.substr("abcdef", 1) == "bcdef")
  assert(mf.substr("abcdef", 1, 3) == "bcd")
  assert(mf.substr("abcdef", 0, 4) == "abcd")
  assert(mf.substr("abcdef", 0, 8) == "abcdef")
  assert(mf.substr("abcdef", -1) == "f")
  assert(mf.substr("abcdef", -2) == "ef")
  assert(mf.substr("abcdef", -3, 1) == "d")
  assert(mf.substr("abcdef", 0, -1) == "abcde")
  assert(mf.substr("abcdef", 2, -1) == "cde")
  assert(mf.substr("abcdef", 4, -4) == "")
  assert(mf.substr("abcdef", -3, -1) == "de")
end

local function test_mf_testfolder()
  assert(mf.testfolder(".") > 0)
  assert(mf.testfolder("/") == 2)
  assert(mf.testfolder("@:\\") <= 0)
end

local function test_mf_trim()
  assert(mf.trim(" abc ")=="abc")
  assert(mf.trim(" abc ",0)=="abc")
  assert(mf.trim(" abc ",1)=="abc ")
  assert(mf.trim(" abc ",2)==" abc")
end

local function test_mf_ucase()
  assert(mf.ucase("FOo БАр")=="FOO БАР")
end

local function test_mf_waitkey()
  assert(mf.waitkey(50,0)=="")
  assert(mf.waitkey(50,1)==0xFFFFFFFF)
end

local function test_mf_size2str()
  assert(mf.size2str(123,0,5)=="  123")
  assert(mf.size2str(123,0,-5)=="123  ")
end

local function test_mf_xlat()
  assert(type(mf.xlat("abc"))=="string")
  assert(mf.xlat("ghzybr")=="пряник")
  assert(mf.xlat("сщьзгеук")=="computer")
end

local function test_mf_beep()
  assert(type(mf.beep())=="boolean")
end

local function test_mf_flock()
  for k=0,2 do assert(type(mf.flock(k,-1))=="number") end
end

local function test_mf_GetMacroCopy()
  assert(type(mf.GetMacroCopy) == "function")
end

local function test_mf_Keys()
  assert(Keys == mf.Keys)
  assert(type(Keys) == "function")

  Keys("Esc F a r Space M a n a g e r Space Ф А Р")
  assert(panel.GetCmdLine() == "Far Manager ФАР")
  Keys("Esc")
  assert(panel.GetCmdLine() == "")
end

local function test_mf_exit()
  assert(exit == mf.exit)
  local N
  mf.postmacro(
    function()
      local function f() N=50; exit(); end
      f(); N=100
    end)
  mf.postmacro(function() Keys"Esc" end)
  far.Message("dummy")
  assert(N == 50)
end

local function test_mf_mmode()
  assert(mmode == mf.mmode)
  assert(1 == mmode(1,-1))
end

local function test_mf_print()
  assert(print == mf.print)
  assert(type(print) == "function")
  -- test on command line
  local str = "abc ABC абв АБВ"
  Keys("Esc")
  print(str)
  assert(panel.GetCmdLine() == str)
  Keys("Esc")
  assert(panel.GetCmdLine() == "")
  -- test on dialog input field
  Keys("F7 CtrlY")
  print(str)
  assert(Dlg.GetValue(-1,0) == str)
  Keys("Esc")
  -- test on editor
  str = "abc ABC\nабв АБВ"
  Keys("ShiftF4")
  print(TmpFileName)
  Keys("Enter CtrlHome Enter Up")
  print(str)
  Keys("CtrlHome"); assert(Editor.Value == "abc ABC")
  Keys("Down");     assert(Editor.Value == "абв АБВ")
  editor.Quit()
end

local function test_mf_postmacro()
  assert(type(mf.postmacro) == "function")
end

local function test_mf_sleep()
  assert(type(mf.sleep) == "function")
end

local function test_mf_usermenu()
  assert(type(mf.usermenu) == "function")
end

function MT.test_mf()
  test_mf_abs()
  test_mf_acall()
  test_mf_akey()
  test_mf_asc()
  test_mf_atoi()
--test_mf_beep()
  test_mf_chr()
  test_mf_clip()
  test_mf_date()
  test_mf_env()
  test_mf_eval()
  test_mf_exit()
  test_mf_fattr()
  test_mf_fexist()
  test_mf_float_and_string()
  test_mf_flock()
  test_mf_fmatch()
  test_mf_fsplit()
  test_mf_GetMacroCopy()
  test_mf_iif()
  test_mf_index()
  test_mf_int()
  test_mf_itoa()
  test_mf_key()
  test_mf_Keys()
  test_mf_lcase()
  test_mf_len()
  test_mf_max()
  test_mf_min()
  test_mf_mmode()
  test_mf_mod()
  test_mf_msave()
  test_mf_msgbox()
  test_mf_postmacro()
  test_mf_print()
  test_mf_prompt()
  test_mf_replace()
  test_mf_rindex()
  test_mf_size2str()
  test_mf_sleep()
  test_mf_strpad()
  test_mf_strwrap()
  test_mf_substr()
  test_mf_testfolder()
  test_mf_trim()
  test_mf_ucase()
--test_mf_usermenu()
  test_mf_waitkey()
  test_mf_xlat()
end

function MT.test_CmdLine()
  Keys"Esc f o o Space Б а р"
  assert(CmdLine.Bof==false)
  assert(CmdLine.Eof==true)
  assert(CmdLine.Empty==false)
  assert(CmdLine.Selected==false)
  assert(CmdLine.Value=="foo Бар")
  assert(CmdLine.ItemCount==7)
  assert(CmdLine.CurPos==8)

  Keys"SelWord"
  assert(CmdLine.Selected)

  Keys"CtrlHome"
  assert(CmdLine.Bof==true)
  assert(CmdLine.Eof==false)

  Keys"Esc"
  assert(CmdLine.Bof==true)
  assert(CmdLine.Eof==true)
  assert(CmdLine.Empty==true)
  assert(CmdLine.Selected==false)
  assert(CmdLine.Value=="")
  assert(CmdLine.ItemCount==0)
  assert(CmdLine.CurPos==1)

  Keys"Esc"
  print("foo Бар")
  assert(CmdLine.Value=="foo Бар")

  Keys"Esc"
  print(("%s %d %s"):format("foo", 5+7, "Бар"))
  assert(CmdLine.Value=="foo 12 Бар")

  Keys"Esc"
end

function MT.test_Far()
  assert(type(Far.FullScreen) == "boolean")
  assert(type(Far.Height) == "number")
  assert(type(Far.IsUserAdmin) == "boolean")
  assert(type(Far.PID) == "number")
  assert(type(Far.Title) == "string")
  assert(type(Far.Width) == "number")

  local temp = Far.UpTime
  mf.sleep(50)
  temp = Far.UpTime - temp
  assert(temp > 40 and temp < 80)
  assert(type(Far.Cfg_Get("Editor","defaultcodepage"))=="number")
  assert(type(Far.DisableHistory)=="function")
  assert(type(Far.KbdLayout(0))=="number")
  assert(type(Far.KeyBar_Show(0))=="number")
  assert(type(Far.Window_Scroll)=="function")

  -- test_Far_GetConfig()
end

local function test_CheckAndGetHotKey()
  mf.acall(far.Menu, {Flags="FMENU_AUTOHIGHLIGHT"},
    {{text="abcd"},{text="abc&d"},{text="abcd"},{text="abcd"},{text="abcd"}})

  assert(Object.CheckHotkey("a")==1)
  assert(Object.GetHotkey(1)=="a")
  assert(Object.GetHotkey()=="a")
  assert(Object.GetHotkey(0)=="a")

  assert(Object.CheckHotkey("b")==3)
  assert(Object.GetHotkey(3)=="b")

  assert(Object.CheckHotkey("c")==4)
  assert(Object.GetHotkey(4)=="c")

  assert(Object.CheckHotkey("d")==2)
  assert(Object.GetHotkey(2)=="d")

  assert(Object.CheckHotkey("e")==0)

  assert(Object.CheckHotkey("")==5)
  assert(Object.GetHotkey(5)=="")
  assert(Object.GetHotkey(6)=="")

  Keys("Esc")
end

function MT.test_Menu()
  Keys("F11")
  assert_str(Menu.Value)
  assert_eq(Menu.Id, far.Guids.PluginsMenuId)
  assert_eq(Menu.Id, "937F0B1C-7690-4F85-8469-AA935517F202")
  Keys("Esc")
end

function MT.test_Object()
  assert(type(Object.Bof)         == "boolean")
  assert(type(Object.CurPos)      == "number")
  assert(type(Object.Empty)       == "boolean")
  assert(type(Object.Eof)         == "boolean")
  assert(type(Object.Height)      == "number")
  assert(type(Object.ItemCount)   == "number")
  assert(type(Object.Selected)    == "boolean")
  assert(type(Object.Title)       == "string")
  assert(type(Object.Width)       == "number")

  test_CheckAndGetHotKey()
end

function MT.test_Drv()
  Keys"AltF1"
  assert(type(Drv.ShowMode) == "number")
  assert(Drv.ShowPos == 1)
  Keys"Esc AltF2"
  assert(type(Drv.ShowMode) == "number")
  assert(Drv.ShowPos == 2)
  Keys"Esc"
end

function MT.test_Help()
  Keys"F1"
  assert(type(Help.FileName)=="string")
  assert(type(Help.SelTopic)=="string")
  assert(type(Help.Topic)=="string")
  Keys"Esc"
end

function MT.test_Mouse()
  assert(type(Mouse.X) == "number")
  assert(type(Mouse.Y) == "number")
  assert(type(Mouse.Button) == "number")
  assert(type(Mouse.CtrlState) == "number")
  assert(type(Mouse.EventFlags) == "number")
  assert(type(Mouse.LastCtrlState) == "number")
end

function MT.test_XPanel(pan) -- (@pan: either APanel or PPanel)
  assert(type(pan.Bof)         == "boolean")
  assert(type(pan.ColumnCount) == "number")
  assert(type(pan.CurPos)      == "number")
  assert(type(pan.Current)     == "string")
  assert(type(pan.DriveType)   == "number")
  assert(type(pan.Empty)       == "boolean")
  assert(type(pan.Eof)         == "boolean")
  assert(type(pan.FilePanel)   == "boolean")
  assert(type(pan.Filter)      == "boolean")
  assert(type(pan.Folder)      == "boolean")
  assert(type(pan.Format)      == "string")
  assert(type(pan.Height)      == "number")
  assert(type(pan.HostFile)    == "string")
  assert(type(pan.ItemCount)   == "number")
  assert(type(pan.Left)        == "boolean")
  assert(type(pan.OPIFlags)    == "number")
  assert(type(pan.Path)        == "string")
  assert(type(pan.Path0)       == "string")
  assert(type(pan.Plugin)      == "boolean")
  assert(type(pan.Prefix)      == "string")
  assert(type(pan.Root)        == "boolean")
  assert(type(pan.SelCount)    == "number")
  assert(type(pan.Selected)    == "boolean")
  assert(type(pan.Type)        == "number")
  assert(type(pan.UNCPath)     == "string")
  assert(type(pan.Visible)     == "boolean")
  assert(type(pan.Width)       == "number")

  if pan == APanel then
    Keys "End"  assert(pan.Eof==true)
    Keys "Home" assert(pan.Bof==true)
  end
end

local function test_Panel_Item()
  for pt=0,1 do
    assert(type(Panel.Item(pt,0,0))  =="string")
    assert(type(Panel.Item(pt,0,1))  =="string")
    assert(type(Panel.Item(pt,0,2))  =="number")
    assert(type(Panel.Item(pt,0,3))  =="string")
    assert(type(Panel.Item(pt,0,4))  =="string")
    assert(type(Panel.Item(pt,0,5))  =="string")
    assert(IsNumOrInt(Panel.Item(pt,0,6)))
    assert(IsNumOrInt(Panel.Item(pt,0,7)))
    assert(type(Panel.Item(pt,0,8))  =="boolean")
    assert(type(Panel.Item(pt,0,9))  =="number")
    assert(type(Panel.Item(pt,0,10)) =="number")
    assert(type(Panel.Item(pt,0,11)) =="string")
    assert(type(Panel.Item(pt,0,12)) =="string")
    assert(type(Panel.Item(pt,0,13)) =="number")
    assert(type(Panel.Item(pt,0,14)) =="number")
    assert(IsNumOrInt(Panel.Item(pt,0,15)))
    assert(IsNumOrInt(Panel.Item(pt,0,16)))
    assert(IsNumOrInt(Panel.Item(pt,0,17)))
    assert(type(Panel.Item(pt,0,18)) =="number")
    assert(IsNumOrInt(Panel.Item(pt,0,19)))
    assert(type(Panel.Item(pt,0,20)) =="string")
    assert(IsNumOrInt(Panel.Item(pt,0,21)))
  end
end

local function test_Panel_SetPath()
  -- store
  local adir_old = panel.GetPanelDirectory(1)
  local pdir_old = panel.GetPanelDirectory(0)
  --test
  local pdir = "/bin"
  local adir = "/usr/bin"
  local afile = "ldd"
  Panel.SetPath(1, pdir)
  Panel.SetPath(0, adir, afile)
  assert(pdir == panel.GetPanelDirectory(0))
  assert(adir == panel.GetPanelDirectory(1))
  assert(panel.GetCurrentPanelItem(1).FileName == afile)
  -- restore
  Panel.SetPath(1, pdir_old)
  Panel.SetPath(0, adir_old)
  actl.Commit()
end

function MT.test_Panel()
  test_Panel_Item()

  assert(Panel.FAttr(0,":")==-1)
  assert(Panel.FAttr(1,":")==-1)

  assert(Panel.FExist(0,":")==0)
  assert(Panel.FExist(1,":")==0)

  assert(type(Panel.Select)    == "function")
  test_Panel_SetPath()
  assert(type(Panel.SetPos)    == "function")
  assert(type(Panel.SetPosIdx) == "function")
end

function MT.test_Dlg()
  Keys"F7 a b c"
  assert(Area.Dialog)
  assert(Dlg.Id == "FAD00DBE-3FFF-4095-9232-E1CC70C67737")
  assert(Dlg.Owner == 0)
  assert(Dlg.ItemCount > 6)
  assert(Dlg.ItemType == 4)
  assert(Dlg.CurPos == 3)
  assert(Dlg.PrevPos == 0)

  Keys"Tab"
  local pos = Dlg.CurPos
  assert(Dlg.CurPos > 3)
  assert(Dlg.PrevPos == 3)
  assert(pos == Dlg.SetFocus(3))
  assert(pos == Dlg.PrevPos)

  assert(Dlg.GetValue(0,0) == Dlg.ItemCount)
  Keys"Esc"
end

function MT.test_Plugin()
  assert(Plugin.Menu()==false)
  assert(Plugin.Config()==false)
  assert(Plugin.Command()==false)
  assert(Plugin.Command(luamacroId)==true)

  assert(Plugin.Exist(far.GetPluginId()) == true)
  assert(Plugin.Exist(far.GetPluginId()+1) == false)

  local function test (func, N) -- Plugin.Call, Plugin.SyncCall: test arguments and returns
    local i1 = bit64.new("0x8765876587658765")
    local r1,r2,r3,r4,r5 = func(luamacroId, "argtest", "foo", i1, -2.34, false, {"foo\0bar"})
    assert(r1=="foo" and r2==i1 and r3==-2.34 and r4==false and type(r5)=="table" and r5[1]=="foo\0bar")

    local src = {}
    for k=1,N do src[k]=k end
    local trg = { func(luamacroId, "argtest", unpack(src)) }
    assert(#trg==N and trg[1]==1 and trg[N]==N)
  end
  test(Plugin.Call, 8000-8)
  test(Plugin.SyncCall, 8000-8)
end

local function test_far_MacroExecute()
  local function test(code, flags)
    local t = far.MacroExecute(code, flags,
      "foo",
      false,
      5,
      nil,
      bit64.new("0x8765876587658765"),
      {"bar"})
    assert(type(t) == "table")
    assert(t.n  == 6)
    assert(t[1] == "foo")
    assert(t[2] == false)
    assert(t[3] == 5)
    assert(t[4] == nil)
    assert(t[5] == bit64.new("0x8765876587658765"))
    assert(type(t[6])=="table" and t[6][1]=="bar")
  end
  test("return ...", nil)
  test("return ...", "KMFLAGS_LUA")
  test("...", "KMFLAGS_MOONSCRIPT")
end

local function test_far_MacroAdd()
  local area, key, descr = "MACROAREA_SHELL", "CtrlA", "Test MacroAdd"

  local Id = far.MacroAdd(area, nil, key, [[A = { b=5 }]], descr)
  assert(type(Id)=="userdata" and far.MacroDelete(Id))

  Id = far.MacroAdd(-1, nil, key, [[A = { b=5 }]], descr)
  assert(not Id) -- bad area

  Id = far.MacroAdd(area, nil, key, [[A = { b:5 }]], descr)
  assert(not Id) -- bad code

  Id = far.MacroAdd(area, "KMFLAGS_MOONSCRIPT", key, [[A = { b:5 }]], descr)
  assert(type(Id)=="userdata" and far.MacroDelete(Id))

  Id = far.MacroAdd(area, "KMFLAGS_MOONSCRIPT", key, [[A = { b=5 }]], descr)
  assert(not Id) -- bad code

  Id = far.MacroAdd(area, nil, key, [[@c:\myscript 5+6,"foo"]], descr)
  assert(type(Id)=="userdata" and far.MacroDelete(Id))

  Id = far.MacroAdd(area, nil, key, [[@c:\myscript 5***6,"foo"]], descr)
  assert(type(Id)=="userdata" and far.MacroDelete(Id)) -- with @ there is no syntax check till the macro runs

  Id = far.MacroAdd(nil, nil, key, [[@c:\myscript]])
  assert(type(Id)=="userdata" and far.MacroDelete(Id)) -- check default area (MACROAREA_COMMON)

  local Id = far.MacroAdd(area,nil,key,[[Keys"F7" assert(Dlg.Id=="FAD00DBE-3FFF-4095-9232-E1CC70C67737") Keys"Esc"]],descr)
  assert(0==mf.eval("Shell/"..key, 2))
  assert(far.MacroDelete(Id))

  Id = far.MacroAdd(area,nil,key,[[a=5]],descr,function(id,flags) return false end)
  assert(-2 == mf.eval("Shell/"..key, 2))
  assert(far.MacroDelete(Id))

  Id = far.MacroAdd(area,nil,key,[[a=5]],descr,function(id,flags) error"deliberate error" end)
  assert(-2 == mf.eval("Shell/"..key, 2))
  assert(far.MacroDelete(Id))

  Id = far.MacroAdd(area,nil,key,[[a=5]],descr,function(id,flags) return id==Id end)
  assert(0 == mf.eval("Shell/"..key, 2))
  assert(far.MacroDelete(Id))

end

local function test_far_MacroCheck()
  assert(far.MacroCheck([[A = { b=5 }]]))
  assert(far.MacroCheck([[A = { b=5 }]], "KMFLAGS_LUA"))

  assert(not far.MacroCheck([[A = { b:5 }]], "KMFLAGS_SILENTCHECK"))

  assert(far.MacroCheck([[A = { b:5 }]], "KMFLAGS_MOONSCRIPT"))

  assert(not far.MacroCheck([[A = { b=5 }]], {KMFLAGS_MOONSCRIPT=1,KMFLAGS_SILENTCHECK=1} ))

  WriteTmpFile [[A = { b=5 }]] -- valid Lua, invalid MoonScript
  assert(far.MacroCheck("@"..TmpFileName, "KMFLAGS_LUA"))
  assert(far.MacroCheck("@"..TmpFileName.." 5+6,'foo'", "KMFLAGS_LUA")) -- valid file arguments
  assert(not far.MacroCheck("@"..TmpFileName.." 5***6,'foo'", "KMFLAGS_SILENTCHECK")) -- invalid file arguments
  assert(not far.MacroCheck("@"..TmpFileName, {KMFLAGS_MOONSCRIPT=1,KMFLAGS_SILENTCHECK=1}))

  WriteTmpFile [[A = { b:5 }]] -- invalid Lua, valid MoonScript
  assert(not far.MacroCheck("@"..TmpFileName, "KMFLAGS_SILENTCHECK"))
  assert(far.MacroCheck("@"..TmpFileName, "KMFLAGS_MOONSCRIPT"))
  DeleteTmpFile()

  assert(not far.MacroCheck([[@//////]], "KMFLAGS_SILENTCHECK"))
end

local function test_far_MacroGetArea()
  assert(far.MacroGetArea()==F.MACROAREA_SHELL)
end

local function test_far_MacroGetLastError()
  assert(far.MacroCheck("a=1"))
  assert(far.MacroGetLastError().ErrSrc=="")
  assert(not far.MacroCheck("a=", "KMFLAGS_SILENTCHECK"))
  assert(far.MacroGetLastError().ErrSrc:len() > 0)
end

local function test_far_MacroGetState()
  local st = far.MacroGetState()
  assert(st==F.MACROSTATE_EXECUTING or st==F.MACROSTATE_EXECUTING_COMMON)
end

local function test_MacroControl()
  test_far_MacroAdd()
  test_far_MacroCheck()
  test_far_MacroExecute()
  test_far_MacroGetArea()
  test_far_MacroGetLastError()
  test_far_MacroGetState()
end

local function test_RegexControl()
  local L = win.Utf8ToUtf32
  local pat = "([bc]+)"
  local pat2 = "([bc]+)|(zz)"
  local rep = "%1%1"
  local R = regex.new(pat)
  local R2 = regex.new(pat2)

  local fr,to,cap
  local str, nfound, nrep

  assert(R:bracketscount()==2)

  fr,to,cap = regex.find("abc", pat)
  assert(fr==2 and to==3 and cap=="bc")
  fr,to,cap = regex.findW(L"abc", pat)
  assert(fr==2 and to==3 and cap==L"bc")

  fr,to,cap = R:find("abc")
  assert(fr==2 and to==3 and cap=="bc")
  fr,to,cap = R:findW(L"abc")
  assert(fr==2 and to==3 and cap==L"bc")

  fr,to,cap = regex.exec("abc", pat2)
  assert(fr==2 and to==3 and #cap==4 and cap[1]==2 and cap[2]==3 and cap[3]==false and cap[4]==false)
  fr,to,cap = regex.execW(L"abc", pat2)
  assert(fr==2 and to==3 and #cap==4 and cap[1]==2 and cap[2]==3 and cap[3]==false and cap[4]==false)

  fr,to,cap = R2:exec("abc")
  assert(fr==2 and to==3 and #cap==4 and cap[1]==2 and cap[2]==3 and cap[3]==false and cap[4]==false)
  fr,to,cap = R2:execW(L"abc")
  assert(fr==2 and to==3 and #cap==4 and cap[1]==2 and cap[2]==3 and cap[3]==false and cap[4]==false)

  assert(regex.match("abc", pat)=="bc")
  assert(regex.matchW(L"abc", pat)==L"bc")

  assert(R:match("abc")=="bc")
  assert(R:matchW(L"abc")==L"bc")

  str, nfound, nrep = regex.gsub("abc", pat, rep)
  assert(str=="abcbc" and nfound==1 and nrep==1)
  str, nfound, nrep = regex.gsubW(L"abc", pat, rep)
  assert(str==L"abcbc" and nfound==1 and nrep==1)

  str, nfound, nrep = R:gsub("abc", rep)
  assert(str=="abcbc" and nfound==1 and nrep==1)
  str, nfound, nrep = R:gsubW(L"abc", rep)
  assert(str==L"abcbc" and nfound==1 and nrep==1)

  local t = {}
  for cap in regex.gmatch("abc", ".") do t[#t+1]=cap end
  assert(#t==3 and t[1]=="a" and t[2]=="b" and t[3]=="c")
  for cap in regex.gmatchW(L"abc", ".") do t[#t+1]=cap end
  assert(#t==6 and t[4]==L"a" and t[5]==L"b" and t[6]==L"c")

  str, nfound, nrep = regex.gsub(";a;", "a*", "ITEM")
  assert(str=="ITEM;ITEM;ITEM" and nfound==3 and nrep==3)
  str, nfound, nrep = regex.gsub(";a;", "a*?", "ITEM")
  assert(str=="ITEM;ITEMaITEM;ITEM" and nfound==4 and nrep==4)
end

--[[------------------------------------------------------------------------------------------------
0001722: DN_EDITCHANGE приходит лишний раз и с ложной информацией

Description:
  [ Far 2.0.1807, Far 3.0.1897 ]
  Допустим диалог состоит из единственного элемента DI_EDIT, больше элементов нет. При появлении
  диалога сразу нажмём на клавишу, допустим, W. Приходят два события DN_EDITCHANGE вместо одного,
  причём в первом из них PtrData указывает на пустую строку.

  Последующие нажатия на клавиши, вызывающие изменения текста, отрабатываются правильно, лишние
  ложные события не приходят.
--]]------------------------------------------------------------------------------------------------
function MT.test_mantis_1722()
  local check = 0
  local function DlgProc (hDlg, msg, p1, p2)
    if msg == F.DN_EDITCHANGE then
      check = check + 1
      assert(p1 == 1)
    end
  end
  local Dlg = { {"DI_EDIT", 3,1,56,10, 0,0,0,0, "a"}, }
  mf.acall(far.Dialog, -1,-1,60,3,"Contents",Dlg, 0, DlgProc)
  assert(Area.Dialog)
  Keys("W 1 2 3 4 BS Esc")
  assert(check == 6)
  assert(Dlg[1][10] == "W123")
end

local function test_utf8_len()
  assert((""):len() == 0)
  assert(("FOo БАр"):len() == 7)
end

local function test_utf8_sub()
  local text = "abcdабвг"
  local len = assert(text:len()==8) and 8

  for _,start in ipairs{-len*3, 0, 1} do
    assert(text:sub(start, -len*4) == "")
    assert(text:sub(start, -len*3) == "")
    assert(text:sub(start, -len*2) == "")
    assert(text:sub(start, -len-1 + 0) == "")
    assert(text:sub(start,          0) == "")
    assert(text:sub(start, -len-1 + 1) == "a")
    assert(text:sub(start,          1) == "a")
    assert(text:sub(start, -len-1 + 6) == "abcdаб")
    assert(text:sub(start,          6) == "abcdаб")
    assert(text:sub(start, len*1) == text)
    assert(text:sub(start, len*2) == text)
  end

  for _,start in ipairs{3, -6} do
    assert(text:sub(start, -len*2)  == "")
    assert(text:sub(start,      0)  == "")
    assert(text:sub(start,      1)  == "")
    assert(text:sub(start, start-1) == "")
    assert(text:sub(start,      -6) == "c")
    assert(text:sub(start, start+0) == "c")
    assert(text:sub(start,      -5) == "cd")
    assert(text:sub(start, start+3) == "cdаб")
    assert(text:sub(start,      -3) == "cdаб")
    assert(text:sub(start, len)     == "cdабвг")
    assert(text:sub(start, 2*len)   == "cdабвг")
  end

  for _,start in ipairs{len+1, 2*len} do
    for _,fin in ipairs{-2*len, -1*len, -1, 0, 1, len-1, len, 2*len} do
      assert(text:sub(start,fin) == "")
    end
  end

  for _,start in ipairs{-2*len,-len-1,-len,-len+1,-1,0,1,len-1,len,len+1} do
    assert(text:sub(start) == text:sub(start,len))
  end

  assert(not pcall(text.sub, text))
  assert(not pcall(text.sub, text, {}))
  assert(not pcall(text.sub, text, nil))
end

local function test_utf8_lower_upper()
  assert((""):lower() == "")
  assert(("abc"):lower() == "abc")
  assert(("ABC"):lower() == "abc")

  assert((""):upper() == "")
  assert(("abc"):upper() == "ABC")
  assert(("ABC"):upper() == "ABC")

  local russian_abc = "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдеёжзийклмнопрстуфхцчшщъыьэюя"
  local part1, part2 = russian_abc:sub(1,33), russian_abc:sub(34)
  assert(part1:lower() == part2)
  assert(part2:lower() == part2)
  assert(part1:upper() == part1)
  assert(part2:upper() == part1)

  local noletters = "1234567890~@#$%^&*()_+-=[]{}|/\\';.,"
  assert(noletters:lower() == noletters)
  assert(noletters:upper() == noletters)
end

---------------------------------------------------------------------------------------------------
-- ACTL_GETWINDOWCOUNT, ACTL_GETWINDOWTYPE, ACTL_GETWINDOWINFO, ACTL_SETCURRENTWINDOW, ACTL_COMMIT
---------------------------------------------------------------------------------------------------
local function test_AdvControl_Window()
  local num, t

  num = far.AdvControl("ACTL_GETWINDOWCOUNT")
  assert(num == 1) -- no "desktop" in Far2
  mf.acall(far.Show); mf.acall(far.Show)
--  assert(far.AdvControl("ACTL_GETSHORTWINDOWINFO").Type == F.WTYPE_VMENU)
--  assert(num+2 == far.AdvControl("ACTL_GETWINDOWCOUNT")) -- menus don't count as windows?
  Keys("Esc Esc")
  assert(num == far.AdvControl("ACTL_GETWINDOWCOUNT"))

  -- Get information about 2 available windows
  t = assert(far.AdvControl("ACTL_GETWINDOWINFO", 1))
  assert(t.Type==F.WTYPE_DESKTOP and t.Id==0 and t.Pos==1 and t.Flags==0 and #t.TypeName>0 and
         t.Name=="")

  t = assert(far.AdvControl("ACTL_GETWINDOWINFO", 2))
  assert(t.Type==F.WTYPE_PANELS and t.Id==0 and t.Pos==2 and t.Flags==F.WIF_CURRENT and
         #t.TypeName>0 and #t.Name>0)
  assert(far.AdvControl("ACTL_GETWINDOWTYPE").Type == F.WTYPE_PANELS)

  -- Set "Desktop" as the current window
  assert(1 == far.AdvControl("ACTL_SETCURRENTWINDOW", 1))
  assert(1 == far.AdvControl("ACTL_COMMIT"))
  t = assert(far.AdvControl("ACTL_GETWINDOWINFO", 2)) -- "z-order": the window that was #1 is now #2
  assert(t.Type==0 and t.Id==0 and t.Pos==2 and t.Flags==F.WIF_CURRENT and #t.TypeName>0 and
         t.Name=="")
  assert(far.AdvControl("ACTL_GETWINDOWTYPE").Type == F.WTYPE_DESKTOP)
  t = assert(far.AdvControl("ACTL_GETWINDOWINFO", 1))
  assert(t.Type==F.WTYPE_PANELS and t.Id==0 and t.Pos==1 and t.Flags==0 and #t.TypeName>0 and
         #t.Name>0)

  -- Restore "Panels" as the current window
  assert(1 == far.AdvControl("ACTL_SETCURRENTWINDOW", 1))
  assert(1 == far.AdvControl("ACTL_COMMIT"))
  assert(far.AdvControl("ACTL_GETWINDOWTYPE").Type == F.WTYPE_PANELS)
end

local function test_AdvControl_Colors()
  local allcolors = assert(far.AdvControl("ACTL_GETARRAYCOLOR"))
  assert(#allcolors == 147)
  for i,color in ipairs(allcolors) do
    assert(far.AdvControl("ACTL_GETCOLOR", i-1) == color)
  end
  assert(not far.AdvControl("ACTL_GETCOLOR", #allcolors))
  assert(not far.AdvControl("ACTL_GETCOLOR", -1))

  -- change the colors
  local arr, elem = {StartIndex=0; Flags=0}, 123
  for n=1,#allcolors do arr[n]=elem end
  assert(far.AdvControl("ACTL_SETARRAYCOLOR", arr))
  for n=1,#allcolors do
    assert(elem == far.AdvControl("ACTL_GETCOLOR", n-1))
  end

  -- restore the colors
  assert(far.AdvControl("ACTL_SETARRAYCOLOR", allcolors))
end

local function test_AdvControl_Misc()
  local t

  assert(type(far.AdvControl("ACTL_GETFARHWND"))=="userdata")

  assert(far.AdvControl("ACTL_GETFARVERSION"):sub(1,1)=="2")
  assert(far.AdvControl("ACTL_GETFARVERSION",true)==2)

  t = far.AdvControl("ACTL_GETFARRECT")
  assert(t.Left>=0 and t.Top>=0 and t.Right>t.Left and t.Bottom>t.Top)

  assert(true == far.AdvControl("ACTL_SETCURSORPOS", {X=-1,Y=0}))
  for k=0,2 do
    assert(true == far.AdvControl("ACTL_SETCURSORPOS", {X=k,Y=k+1}))
    t = assert(far.AdvControl("ACTL_GETCURSORPOS"))
    assert(t.X==k and t.Y==k+1)
  end

  assert(true == mf.acall(far.AdvControl, "ACTL_WAITKEY", "KEY_F4"))
  Keys("F4")
  assert(true == mf.acall(far.AdvControl, "ACTL_WAITKEY"))
  Keys("F2")
end

local function test_ACTL()
  assert_func  ( actl.Commit)
  assert_func  ( actl.EjectMedia)
  assert_tbl   ( actl.GetArrayColor())
  assert_range ( #actl.GetArrayColor(),142,152)
  assert_num   ( actl.GetColor("COL_DIALOGBOXTITLE"))
  assert_num   ( actl.GetConfirmations())
  assert_tbl   ( actl.GetCursorPos())
  assert_num   ( actl.GetDescSettings())
  assert_num   ( actl.GetDialogSettings())
  assert_eq    ( type(actl.GetFarHwnd()), "userdata")
  assert_tbl   ( actl.GetFarRect())
  assert_str   ( actl.GetFarVersion())
  assert_num   ( actl.GetFarVersion(true))
  assert_num   ( actl.GetInterfaceSettings())
  assert_num   ( actl.GetPanelSettings())
  assert_range ( actl.GetPluginMaxReadData(), 0x1000, 0x80000)
  assert_num   ( actl.GetSystemSettings())
  assert_str   ( actl.GetSysWordDiv())
  assert_range ( actl.GetWindowCount(), 1)
  assert_tbl   ( actl.GetWindowInfo(1))
  assert_tbl   ( actl.GetShortWindowInfo(1))
  assert_nil   ( actl.KeyMacro)
  assert_func  ( actl.ProgressNotify)
  assert_func  ( actl.Quit)
  assert_func  ( actl.RedrawAll)
  assert_func  ( actl.SetArrayColor)
  assert_func  ( actl.SetCurrentWindow)
  assert_func  ( actl.SetCursorPos)
  assert_func  ( actl.SetProgressState)
  assert_func  ( actl.SetProgressValue)
  assert_nil   ( actl.Synchro)
  assert_func  ( actl.WaitKey)
end

local function test_AdvControl()
--test_AdvControl_Window()
  test_AdvControl_Colors()
  test_AdvControl_Misc()
  test_ACTL()
end

local function test_far_GetMsg()
  assert(type(far.GetMsg(0))=="string")
end

local function test_clipboard()
  local orig = far.PasteFromClipboard()
  local values = { "Человек", "foo", "", n=3 }
  for k=1,values.n do
    local v = values[k]
    far.CopyToClipboard(v)
    assert(far.PasteFromClipboard() == v)
  end
  if orig then far.CopyToClipboard(orig) end
  assert(far.PasteFromClipboard() == orig)
end

local function test_far_FarClock()
  -- check time difference
  local temp = far.FarClock()
  win.Sleep(500)
  temp = (far.FarClock() - temp) / 1000
  assert(temp > 480 and temp < 550, temp)
  -- check granularity
  local OK = false
  temp = far.FarClock() % 10
  for k=1,10 do
    win.Sleep(20)
    if temp ~= far.FarClock() % 10 then OK=true; break; end
  end
  assert(OK)
end

local function test_ProcessName()
  assert_true  (far.CheckMask("f*.ex?"))
  assert_true  (far.CheckMask("/(abc)?def/"))
  assert_false (far.CheckMask("/[[[/"))

  assert_eq    (far.GenerateName("a??b.*", "cdef.txt"), "adeb.txt")

  assert_true  (far.CmpName("f*.ex?",      "ftp.exe"        ))
  assert_true  (far.CmpName("f*.ex?",      "fc.exe"         ))
  assert_true  (far.CmpName("f*.ex?",      "f.ext"          ))
  assert_false (far.CmpName("f*.ex?",      "a/f.ext"        ))
  assert_false (far.CmpName("f*.ex?",      "a/f.ext", 0     ))
  assert_true  (far.CmpName("f*.ex?",      "a/f.ext", "PN_SKIPPATH" ))

  assert_true  (far.CmpName("*co*",        "color.ini"      ))
  assert_true  (far.CmpName("*co*",        "edit.com"       ))
  assert_true  (far.CmpName("[c-ft]*.txt", "config.txt"     ))
  assert_true  (far.CmpName("[c-ft]*.txt", "demo.txt"       ))
  assert_true  (far.CmpName("[c-ft]*.txt", "faq.txt"        ))
  assert_true  (far.CmpName("[c-ft]*.txt", "tips.txt"       ))
  assert_true  (far.CmpName("*",           "foo.bar"        ))
  assert_true  (far.CmpName("*.cpp",       "foo.cpp"        ))
  assert_false (far.CmpName("*.cpp",       "foo.abc"        ))
  assert_false (far.CmpName("*|*.cpp",     "foo.abc"        )) -- exclude mask not supported
  assert_false (far.CmpName("*,*",         "foo.bar"        )) -- mask list not supported

  assert_true (far.CmpNameList("*",          "foo.bar"    ))
  assert_true (far.CmpNameList("*.cpp",      "foo.cpp"    ))
  assert_false(far.CmpNameList("*.cpp",      "foo.abc"    ))
  assert_true (far.CmpNameList("*|*.cpp",    "foo.abc"    )) -- exclude mask IS supported
  assert_true (far.CmpNameList("|*.cpp",     "foo.abc"    )) -- +++
  assert_false(far.CmpNameList("*|*.abc",    "foo.abc"    )) -- +++
  assert_true (far.CmpNameList("*.aa,*.bar", "foo.bar"    ))
  assert_true (far.CmpNameList("*.aa,*.bar", "c:/foo.bar" ))
  assert_true (far.CmpNameList("/.+/",       "c:/foo.bar" ))
  assert_true (far.CmpNameList("/bar$/",     "c:/foo.bar" ))
  assert_false(far.CmpNameList("/dar$/",     "c:/foo.bar" ))
--  assert_true (far.CmpNameList("/abcd/*",    "/abcd/foo.bar"))
  assert_false(far.CmpNameList("/abcd/;*",    "/abcd/foo.bar", "PN_SKIPPATH"))
  assert_true (far.CmpNameList("/Makefile(.+)?/", "Makefile"))
  assert_true (far.CmpNameList("/makefile([._\\-].+)?$/i", "Makefile", "PN_SKIPPATH"))
end

local function test_FarStandardFunctions()
  test_clipboard()
--  test_far_FarClock()

  test_ProcessName()

  assert(far.ConvertPath([[/foo/bar/../../abc]], "CPM_FULL") == [[/abc]])

--  assert(far.FormatFileSize(123456, 8)  == "  123456")
--  assert(far.FormatFileSize(123456, -8) == "123456  ")

  assert(type(far.GetCurrentDirectory()) == "string")

  assert(far.GetPathRoot[[/foo/bar]] == [[/]])

  assert(far.LIsAlpha("A") == true)
  assert(far.LIsAlpha("Я") == true)
  assert(far.LIsAlpha("7") == false)
  assert(far.LIsAlpha(";") == false)

  assert(far.LIsAlphanum("A") == true)
  assert(far.LIsAlphanum("Я") == true)
  assert(far.LIsAlphanum("7") == true)
  assert(far.LIsAlphanum(";") == false)

  assert(far.LIsLower("A") == false)
  assert(far.LIsLower("a") == true)
  assert(far.LIsLower("Я") == false)
  assert(far.LIsLower("я") == true)
  assert(far.LIsLower("7") == false)
  assert(far.LIsLower(";") == false)

  assert(far.LIsUpper("A") == true)
  assert(far.LIsUpper("a") == false)
  assert(far.LIsUpper("Я") == true)
  assert(far.LIsUpper("я") == false)
  assert(far.LIsUpper("7") == false)
  assert(far.LIsUpper(";") == false)

  assert(far.LLowerBuf("abc-ABC-эюя-ЭЮЯ-7;") == "abc-abc-эюя-эюя-7;")
  assert(far.LUpperBuf("abc-ABC-эюя-ЭЮЯ-7;") == "ABC-ABC-ЭЮЯ-ЭЮЯ-7;")

  assert(far.LStricmp("abc","def") < 0)
  assert(far.LStricmp("def","abc") > 0)
  assert(far.LStricmp("abc","abc") == 0)
  assert(far.LStricmp("ABC","def") < 0)
  assert(far.LStricmp("DEF","abc") > 0)
  assert(far.LStricmp("ABC","abc") == 0)

  assert(far.LStrnicmp("abc","def",3) < 0)
  assert(far.LStrnicmp("def","abc",3) > 0)
  assert(far.LStrnicmp("abc","abc",3) == 0)
  assert(far.LStrnicmp("ABC","def",3) < 0)
  assert(far.LStrnicmp("DEF","abc",3) > 0)
  assert(far.LStrnicmp("ABC","abc",3) == 0)
  assert(far.LStrnicmp("111abc","111def",3) == 0)
  assert(far.LStrnicmp("111abc","111def",4) < 0)
end

-- "Several lines are merged into one".
local function test_issue_3129()
  local fname = "/tmp/far2l-"..win.Uuid(win.Uuid()):sub(1,8)
  local fp = assert(io.open(fname, "w"))
  fp:close()
  local flags = {EF_NONMODAL=1, EF_IMMEDIATERETURN=1, EF_DISABLEHISTORY=1}
  assert(editor.Editor(fname,nil,nil,nil,nil,nil,flags) == F.EEC_MODIFIED)
  for k=1,3 do
    editor.InsertString()
    editor.SetString(k, "foo")
  end
  assert(editor.SaveFile())
  assert(editor.Quit())
  actl.Commit()
  fp = assert(io.open(fname))
  local k = 0
  for line in fp:lines() do
    k = k + 1
    assert(line=="foo")
  end
  fp:close()
  win.DeleteFile(fname)
  assert(k == 3)
end

local function test_gmatch_coro()
  local function yieldFirst(it)
    return coroutine.wrap(function()
      coroutine.yield(it())
    end)
  end

  local it = ("1 2 3"):gmatch("(%d+)")
  local head = yieldFirst(it)
  assert(head() == "1")
end

function MT.test_luafar()
  test_bit64()
  test_gmatch_coro()
  test_utf8_len()
  test_utf8_sub()
  test_utf8_lower_upper()

  test_AdvControl()
  test_far_GetMsg()
  test_FarStandardFunctions()
  test_issue_3129()
  test_MacroControl()
  test_RegexControl()
end

-- Test in particular that Plugin.Call (a so-called "restricted" function) works properly
-- from inside a deeply nested coroutine.
local function test_coroutine()
  for k=1,2 do
    local Call = k==1 and Plugin.Call or Plugin.SyncCall
    local function f1()
      coroutine.yield(Call(luamacroId, "argtest", 1, false, "foo", nil))
    end
    local function f2() return coroutine.resume(coroutine.create(f1)) end
    local function f3() return coroutine.resume(coroutine.create(f2)) end
    local function f4() return coroutine.resume(coroutine.create(f3)) end
    local t = pack(f4())
    assert(t.n==7 and t[1]==true and t[2]==true and t[3]==true and
           t[4]==1 and t[5]==false and t[6]=="foo" and t[7]==nil)
  end
end

function MT.test_misc()
  test_coroutine()
end

function MT.test_all()
  TestArea("Shell", "Run these tests from the Shell area.")
  assert(not APanel.Plugin and not PPanel.Plugin, "Run these tests when neither of panels is a plugin panel.")

  MT.test_areas()
  MT.test_mf()
  MT.test_CmdLine()
  MT.test_Help()
  MT.test_Dlg()
  MT.test_Drv()
  MT.test_Far()
  MT.test_Menu()
  MT.test_Mouse()
  MT.test_Object()
  MT.test_Panel()
  MT.test_Plugin()
  MT.test_XPanel(APanel)
  MT.test_XPanel(PPanel)
  MT.test_mantis_1722()
  MT.test_luafar()
  MT.test_misc()
end

return MT
