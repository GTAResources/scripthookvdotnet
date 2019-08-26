/**
 * Copyright (C) 2015 crosire
 *
 * This software is  provided 'as-is', without any express  or implied  warranty. In no event will the
 * authors be held liable for any damages arising from the use of this software.
 * Permission  is granted  to anyone  to use  this software  for  any  purpose,  including  commercial
 * applications, and to alter it and redistribute it freely, subject to the following restrictions:
 *
 *   1. The origin of this software must not be misrepresented; you must not claim that you  wrote the
 *      original  software. If you use this  software  in a product, an  acknowledgment in the product
 *      documentation would be appreciated but is not required.
 *   2. Altered source versions must  be plainly  marked as such, and  must not be  misrepresented  as
 *      being the original software.
 *   3. This notice may not be removed or altered from any source distribution.
 */

#include "Native.hpp"

#include "Blip.hpp"
#include "Camera.hpp"
#include "Entity.hpp"
#include "Ped.hpp"
#include "Player.hpp"
#include "Prop.hpp"
#include "Rope.hpp"
#include "Vector2.hpp"
#include "Vector3.hpp"
#include "Vehicle.hpp"

#using "ScriptHookVDotNet.asi"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Linq::Expressions;
using namespace System::Reflection::Emit;
using namespace System::Runtime::InteropServices;

namespace GTA
{
	namespace Native
	{
		namespace
		{
			UInt64 ObjectToNative(Object ^value)
			{
				if (Object::ReferenceEquals(value, nullptr))
				{
					return 0;
				}

				Type ^type = value->GetType();

				// Fundamental types
				if (type == Boolean::typeid)
				{
					return static_cast<bool>(value) ? System::UInt64(1) : System::UInt64(0);
				}
				if (type == Int32::typeid)
				{
					//As long as the type is correct this won't be problem but prevent value from changing memory expression, in case the type is incorrect
					return static_cast<unsigned int>(static_cast<int>(value));
				}
				if (type == UInt32::typeid)
				{
					return static_cast<unsigned int>(value);
				}
				if (type == Single::typeid)
				{
					float valueFloat = static_cast<float>(value);
					unsigned int valueUInt32 = reinterpret_cast<System::UInt32&>(valueFloat);
					return valueUInt32;
				}
				if (type == Double::typeid)
				{
					float valueFloat = static_cast<float>(static_cast<double>(value));
					unsigned int valueUInt32 = reinterpret_cast<System::UInt32&>(valueFloat);
					return valueUInt32;
				}
				if (type == IntPtr::typeid)
				{
					return static_cast<IntPtr>(value).ToInt64();
				}
				if (type == String::typeid)
				{
					return SHVDN::ScriptDomain::CurrentDomain->PinString(static_cast<String ^>(value)).ToInt64();
				}

				// Scripting types
				if (type == Model::typeid)
				{
					return static_cast<Model>(value).Hash;
				}
				if (IHandleable::typeid->IsAssignableFrom(type))
				{
					return safe_cast<IHandleable ^>(value)->Handle;
				}

				throw gcnew InvalidCastException(String::Concat("Unable to cast object of type '", type->FullName, "' to native value"));
			}
			Object ^ObjectFromNative(Type ^type, UInt64 *value)
			{
				if (type == String::typeid)
				{
					if (*value != 0)
					{
						int size = 0;
						for (const char *c = reinterpret_cast<const char *>(*value); *c; ++c)
							++size;

						const auto bytes = gcnew array<Byte>(size);

						Runtime::InteropServices::Marshal::Copy(static_cast<IntPtr>(static_cast<Int64>(*value)), bytes, 0, size);

						return Text::Encoding::UTF8->GetString(bytes);
					}
					else
					{
						return String::Empty;
					}
				}

				const int handle = *reinterpret_cast<int *>(value);

				// Scripting types
				if (type == Blip::typeid)
				{
					return gcnew Blip(handle);
				}
				if (type == Camera::typeid)
				{
					return gcnew Camera(handle);
				}
				if (type == Entity::typeid)
				{
					if (Function::Call<bool>(Hash::DOES_ENTITY_EXIST, handle))
					{
						switch (Function::Call<int>(Hash::GET_ENTITY_TYPE, handle))
						{
							case 1:
								return gcnew Ped(handle);
							case 2:
								return gcnew Vehicle(handle);
							case 3:
								return gcnew Prop(handle);
						}
					}

					return nullptr;
				}
				if (type == Ped::typeid)
				{
					return gcnew Ped(handle);
				}
				if (type == PedGroup::typeid)
				{
					return gcnew PedGroup(handle);
				}
				if (type == Player::typeid)
				{
					return gcnew Player(handle);
				}
				if (type == Prop::typeid)
				{
					return gcnew Prop(handle);
				}
				if (type == Rope::typeid)
				{
					return gcnew Rope(handle);
				}
				if (type == Vehicle::typeid)
				{
					return gcnew Vehicle(handle);
				}

				throw gcnew InvalidCastException(String::Concat("Unable to cast native value to object of type '", type->FullName, "'"));
			}
		}
		private ref class NativeHelper abstract sealed
		{
		internal:
			value class NativeVector3
			{
			internal:
				float x;
				int _paddingx;
				float y;
				int _paddingy;
				float z;
				int _paddingz;

				static explicit operator GTA::Math::Vector2(NativeVector3 val)
				{
					return GTA::Math::Vector2(val.x, val.y);
				}
				static explicit operator GTA::Math::Vector3(NativeVector3 val)
				{
					return GTA::Math::Vector3(val.x, val.y, val.z);
				}
			};
		};
		generic <typename T>
		T NativeHelperGeneric<T>::ObjectFromNativeGeneric(UInt64 *value)
		{
			// Fundamental types
			if (T::typeid == Boolean::typeid)
			{
				auto valueUInt64 = NativeHelperGeneric<UInt64>::PtrToStructure(System::IntPtr(value));

				// Return proper bool values (don't return true whose internal value is not 1)
				if (valueUInt64 != 0)
				{
					bool properTrueValue = true;
					return PtrToStructure(System::IntPtr(&properTrueValue));
				}
				else
				{
					// The value is 0 so direct convert to bool won't be a problem
					return PtrToStructure(System::IntPtr(value));
				}
			}
			if (T::typeid == Int32::typeid)
			{
				return PtrToStructure(System::IntPtr(value));
			}
			if (T::typeid == UInt32::typeid)
			{
				return PtrToStructure(System::IntPtr(value));
			}
			if (T::typeid == Int64::typeid)
			{
				return PtrToStructure(System::IntPtr(value));
			}
			if (T::typeid == UInt64::typeid)
			{
				return PtrToStructure(System::IntPtr(value));
			}
			if (T::typeid == Single::typeid)
			{
				return PtrToStructure(System::IntPtr(value));
			}
			if (T::typeid == Double::typeid)
			{
				return NativeHelperGeneric<T>::Convert(NativeHelperGeneric<float>::PtrToStructure(System::IntPtr(value)));
			}

			// Math types
			if (T::typeid == Math::Vector2::typeid)
			{
				const auto vec = *reinterpret_cast<NativeHelper::NativeVector3 *> (value);
				return NativeHelperGeneric<T>::Convert(vec);
			}
			if (T::typeid == Math::Vector3::typeid)
			{
				const auto vec = *reinterpret_cast<NativeHelper::NativeVector3 *> (value);
				return NativeHelperGeneric<T>::Convert(vec);
			}

			throw gcnew InvalidCastException(String::Concat("Unable to cast native value to object of type '", T::typeid, "'"));
		}


		InputArgument::InputArgument(UInt64 value) : _data(value)
		{
		}
		InputArgument::InputArgument(Object ^value) : _data(ObjectToNative(value))
		{
		}
		OutputArgument::OutputArgument() : _storage(Marshal::AllocCoTaskMem(24)), InputArgument(_storage)
		{
		}
		OutputArgument::OutputArgument(Object ^value) : OutputArgument()
		{
			*static_cast<UInt64 *>(_storage.ToPointer()) = ObjectToNative(value);
		}
		OutputArgument::~OutputArgument()
		{
			this->!OutputArgument();
		}
		OutputArgument::!OutputArgument()
		{
			Marshal::FreeCoTaskMem(_storage);
		}

		generic <typename T>
		T OutputArgument::GetResult()
		{
			System::Type^ type = T::typeid;
			if (type->IsPrimitive || type == Math::Vector3::typeid || type == Math::Vector2::typeid)
			{
				return NativeHelperGeneric<T>::ObjectFromNativeGeneric(reinterpret_cast<UInt64 *>(_data));
			}
			else
			{
				return static_cast<T>(ObjectFromNative(T::typeid, reinterpret_cast<UInt64 *>(_data)));
			}
		}

		generic <typename T>
		T Function::Call(Hash hash, ... array<InputArgument ^> ^arguments)
		{
			return Call<T>(static_cast<UInt64>(hash), arguments);
		}
		void Function::Call(Hash hash, ... array<InputArgument ^> ^arguments)
		{
			Call<int>(static_cast<UInt64>(hash), arguments);
		}
		generic <typename T>
		T Function::Call(UInt64 hash, ... array<InputArgument ^> ^arguments)
		{
			array<unsigned long long> ^args = gcnew array<unsigned long long>(arguments->Length);
			for (int i = 0; i < args->Length; ++i)
				args[i] = arguments[i]->_data;

			const auto task = gcnew SHVDN::NativeFunc(hash, args);
			SHVDN::ScriptDomain::CurrentDomain->ExecuteTask(task);

			//The result will be null when this method is called from a thread other than the main thread
			if (task->Result == nullptr)
			{
				throw gcnew InvalidOperationException("Native.Function.Call can only be called from the main thread.");
			}

			System::Type^ type = T::typeid;
			if (type->IsPrimitive || type == Math::Vector3::typeid || type == Math::Vector2::typeid)
			{
				return NativeHelperGeneric<T>::ObjectFromNativeGeneric(task->Result);
			}
			else
			{
				return static_cast<T>(ObjectFromNative(T::typeid, task->Result));
			}
		}

		generic<typename T>
		T NativeHelperGeneric<T>::PtrToStructure(System::IntPtr ptr)
		{
			return _ptrToStrFunc(ptr);
		}
		generic<typename T>
		NativeHelperGeneric<T>::NativeHelperGeneric()
		{
			auto ptrToStrMethod = gcnew DynamicMethod("PtrToStructure<" + T::typeid + ">", T::typeid,
				gcnew array<Type ^>{ System::IntPtr::typeid }, NativeHelperGeneric<T>::typeid, true);

			ILGenerator ^generator = ptrToStrMethod->GetILGenerator();
			generator->Emit(OpCodes::Ldarg_0);
			generator->Emit(OpCodes::Ldobj, T::typeid);
			generator->Emit(OpCodes::Ret);

			_ptrToStrFunc = (System::Func<System::IntPtr, T> ^)ptrToStrMethod->CreateDelegate(System::Func<System::IntPtr, T>::typeid);
		}
		generic<typename To>
		generic<typename From>
		To NativeHelperGeneric<To>::Convert(From from)
		{
			return NativeHelperGeneric<To>::CastCache<From>::Convert(from);
		}
		generic<typename To>
		generic<typename From>
		NativeHelperGeneric<To>::CastCache<From>::CastCache()
		{
			auto param = Expression::Parameter(From::typeid);
			auto convert = Expression::Convert(param, To::typeid);
			Convert = Expression::Lambda<System::Func<From, To>^>(convert, param)->Compile();
		}

		int GetUtf8CodePointSize(System::String ^string, int index)
		{
			unsigned int chr = static_cast<unsigned int>(string[index]);

			if (chr < 0x80)
			{
				return 1;
			}
			else if (chr < 0x800)
			{
				return 2;
			}
			else if (chr < 0x10000)
			{
				return 3;
			}
			else
			{
#pragma region Surrogate check
				int temp1 = static_cast<int>(chr) - 0xD800;
				if (temp1 >= 0 && temp1 <= 0x7ff)
				{
					// Found a high surrogate
					if (index < string->Length - 1)
					{
						int temp2 = (int)string[index + 1] - 0xD800;
						if (temp2 >= 0 && temp2 <= 0x3ff)
						{
							// Found a low surrogate
							return 4;
						}
						else
						{
							return 0;
						}
					}
					else
					{
						return 0;
					}
				}
#pragma endregion
			}

			return 0;
		}
		void Function::PushLongString(System::String ^string)
		{
			PushLongString(string, 99);
		}
		void Function::PushLongString(System::String ^string, int maxLengthUtf8)
		{
			if (maxLengthUtf8 <= 0)
			{
				throw gcnew ArgumentOutOfRangeException("maxLengthUtf8");
			}

			const int size = Text::Encoding::UTF8->GetByteCount(string);

			if (size <= maxLengthUtf8)
			{
				Call(Native::Hash::_ADD_TEXT_COMPONENT_STRING, string);
				return;
			}

			int currentUtf8StrLength = 0;
			int startPos = 0;
			int currentPos;

			for (currentPos = 0; currentPos < string->Length; currentPos++)
			{
				int codePointSize = GetUtf8CodePointSize(string, currentPos);

				if (currentUtf8StrLength + codePointSize > maxLengthUtf8)
				{
					Call(Native::Hash::_ADD_TEXT_COMPONENT_STRING, string->Substring(startPos, currentPos - startPos));

					currentUtf8StrLength = 0;
					startPos = currentPos;
				}
				else
				{
					currentUtf8StrLength += codePointSize;
				}

				//if the code point size is 4, additional increment is needed
				if (codePointSize == 4)
				{
					currentPos++;
				}
			}

			Call(Native::Hash::_ADD_TEXT_COMPONENT_STRING, string->Substring(startPos, string->Length - startPos));
		}
	}
}