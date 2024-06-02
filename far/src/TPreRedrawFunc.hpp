#pragma once

/*
TPreRedrawFunc.hpp

Фоновый апдейт

*/
/*
Copyright (c) 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <WinCompat.h>

typedef void (*PREREDRAWFUNC)();

struct PreRedrawParamStruct
{
	DWORD Flags;
	const void *Param1;
	const void *Param2;
	const void *Param3;
	const void *Param4;
	int64_t Param5;
};

struct PreRedrawItem
{
	PREREDRAWFUNC PreRedrawFunc;
	PreRedrawParamStruct Param;
};

class TPreRedrawFunc
{
	private:
		std::vector<PreRedrawItem> Items;

		static PreRedrawItem errorStack;

	public:
		TPreRedrawFunc() {}
		~TPreRedrawFunc() {}

		// вернуть количество элементов на стеке
		unsigned int Size() const { return Items.size(); }

		// взять элемент со стека
		PreRedrawItem Pop();

		// взять элемент со стека без изменения стека
		PreRedrawItem Peek() const;

		// положить элемент на стек
		PreRedrawItem Push(const PreRedrawItem &Source);
		PreRedrawItem Push(PREREDRAWFUNC Func, PreRedrawParamStruct *Param=nullptr);

		PreRedrawItem SetParam(const PreRedrawParamStruct &Param);

		// очистить стек
		void Free() { Items.clear(); }

		bool isEmpty() const { return Items.empty(); }
};

extern TPreRedrawFunc PreRedraw;


class TPreRedrawFuncGuard
{
	public:
		TPreRedrawFuncGuard(PREREDRAWFUNC Func);
		~TPreRedrawFuncGuard();
};
