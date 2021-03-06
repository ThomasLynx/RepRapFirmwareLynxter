/****************************************************************************************************

RepRapFirmware - Main Include

This includes all the other include files in the right order and defines some globals.
No other definitions or information should be in here.

-----------------------------------------------------------------------------------------------------

Version 0.1

18 November 2012

Adrian Bowyer
RepRap Professional Ltd
http://reprappro.com

Licence: GPL

****************************************************************************************************/

#ifndef REPRAPFIRMWARE_H
#define REPRAPFIRMWARE_H

#include <cstddef>		// for size_t
#include <cfloat>
#include <cstdarg>
#include <climits>		// for CHAR_BIT

#include "ecv.h"
#include "Core.h"

typedef uint16_t PwmFrequency;		// type used to represent a PWM frequency. 0 sometimes means "default".

#include "Configuration.h"
#include "Pins.h"

#include "Libraries/General/SafeStrtod.h"
#include "Libraries/General/SafeVsnprintf.h"
#include "Libraries/General/StringRef.h"

// Module numbers and names, used for diagnostics and debug
enum Module : uint8_t
{
	modulePlatform = 0,
	moduleNetwork = 1,
	moduleWebserver = 2,
	moduleGcodes = 3,
	moduleMove = 4,
	moduleHeat = 5,
	moduleDda = 6,
	moduleRoland = 7,
	moduleScanner = 8,
	modulePrintMonitor = 9,
	moduleStorage = 10,
	modulePortControl = 11,
	moduleDuetExpansion = 12,
	moduleFilamentSensors = 13,
	moduleWiFi = 14,
	moduleDisplay = 15,
	moduleLynx = 16,
	numModules = 17,				// make this one greater than the last module number
	noModule = 17
};

extern const char * const moduleName[];

// Warn of what's to come, so we can use pointers to classes without including the entire header files
class Network;
class Platform;
class GCodes;
class Move;
class DDA;
class Kinematics;
class Heat;
class PID;
class TemperatureSensor;
class Tool;
class Roland;
class Scanner;
class PrintMonitor;
class RepRap;
class FileStore;
class OutputBuffer;
class OutputStack;
class GCodeBuffer;
class GCodeQueue;
class FilamentMonitor;
class RandomProbePointSet;
class Logger;

#if SUPPORT_IOBITS
class PortControl;
#endif

#if SUPPORT_12864_LCD
class Display;
#endif

// Define floating point type to use for calculations where we would like high precision in matrix calculations
#if SAM4E || SAM4S || SAME70
typedef double floatc_t;					// type of matrix element used for calibration
#else
// We are more memory-constrained on the SAM3X
typedef float floatc_t;						// type of matrix element used for calibration
#endif

typedef uint32_t AxesBitmap;				// Type of a bitmap representing a set of axes
typedef uint32_t DriversBitmap;				// Type of a bitmap representing a set of driver numbers
typedef uint32_t FansBitmap;				// Type of a bitmap representing a set of fan numbers

// A single instance of the RepRap class contains all the others
extern RepRap reprap;

// Debugging support
extern "C" void debugPrintf(const char* fmt, ...) __attribute__ ((format (printf, 1, 2)));
#define DEBUG_HERE do { } while (false)
//#define DEBUG_HERE do { debugPrintf("At " __FILE__ " line %d\n", __LINE__); delay(50); } while (false)

// Functions and globals not part of any class

#ifdef RTOS

void delay(uint32_t ms);

#else

inline void delay(uint32_t ms)
{
	coreDelay(ms);
}

#endif

bool StringEndsWith(const char* string, const char* ending);
bool StringStartsWith(const char* string, const char* starting);
bool StringEquals(const char* s1, const char* s2);
int StringContains(const char* string, const char* match);
void SafeStrncpy(char *dst, const char *src, size_t length) pre(length != 0);
void SafeStrncat(char *dst, const char *src, size_t length) pre(length != 0);

double HideNan(float val);

void ListDrivers(const StringRef& str, DriversBitmap drivers);

// Macro to assign an array from an initialiser list
#define ARRAY_INIT(_dest, _init) static_assert(sizeof(_dest) == sizeof(_init), "Incompatible array types"); memcpy(_dest, _init, sizeof(_init));

// UTF8 code for the degree-symbol
#define DEGREE_SYMBOL	"\xC2\xB0"	// Unicode degree-symbol as UTF8

// Classes to facilitate range-based for loops that iterate from 0 up to just below a limit
template<class T> class SimpleRangeIterator
{
public:
	SimpleRangeIterator(T value_) : val(value_) {}
    bool operator != (SimpleRangeIterator<T> const& other) const { return val != other.val;     }
    T const& operator*() const { return val; }
    SimpleRangeIterator& operator++() { ++val; return *this; }

private:
    T val;
};

template<class T> class SimpleRange
{
public:
	SimpleRange(T limit) : _end(limit) {}
	SimpleRangeIterator<T> begin() const { return SimpleRangeIterator<T>(0); }
	SimpleRangeIterator<T> end() const { return SimpleRangeIterator<T>(_end); 	}

private:
	const T _end;
};

// Macro to create a SimpleRange from an array
#define ARRAY_INDICES(_arr) (SimpleRange<size_t>(ARRAY_SIZE(_arr)))

// Helper functions to work on bitmaps of various lengths.
// The primary purpose of these is to allow us to switch between 16, 32 and 64-bit bitmaps.

// Convert an unsigned integer to a bit in a bitmap
template<typename BitmapType> inline constexpr BitmapType MakeBitmap(unsigned int n)
{
	return (BitmapType)1u << n;
}

// Make a bitmap with the lowest n bits set
template<typename BitmapType> inline constexpr BitmapType LowestNBits(unsigned int n)
{
	return ((BitmapType)1u << n) - 1;
}

// Check if a particular bit is set in a bitmap
template<typename BitmapType> inline constexpr bool IsBitSet(BitmapType b, unsigned int n)
{
	return (b & ((BitmapType)1u << n)) != 0;
}

// Set a bit in a bitmap
template<typename BitmapType> inline void SetBit(BitmapType &b, unsigned int n)
{
	b |= ((BitmapType)1u << n);
}

// Clear a bit in a bitmap
template<typename BitmapType> inline void ClearBit(BitmapType &b, unsigned int n)
{
	b &= ~((BitmapType)1u << n);
}

// Convert an array of longs to a bit map with overflow checking
template<typename BitmapType> BitmapType UnsignedArrayToBitMap(const uint32_t *arr, size_t numEntries)
{
	BitmapType res = 0;
	for (size_t i = 0; i < numEntries; ++i)
	{
		const uint32_t f = arr[i];
		if (f < sizeof(BitmapType) * CHAR_BIT)
		{
			SetBit(res, f);
		}
	}
	return res;
}

// Common definitions used by more than one module
constexpr uint32_t StepClockRate = VARIANT_MCK/128;					// the frequency of the clock used for stepper pulse timing (see Platform::InitialiseInterrupts)
constexpr uint64_t StepClockRateSquared = (uint64_t)StepClockRate * StepClockRate;

constexpr size_t ScratchStringLength = 220;							// standard length of a scratch string, enough to print delta parameters to
constexpr size_t ShortScratchStringLength = 50;

constexpr size_t XYZ_AXES = 3;										// The number of Cartesian axes
constexpr size_t X_AXIS = 0, Y_AXIS = 1, Z_AXIS = 2, E0_AXIS = 3;	// The indices of the Cartesian axes in drive arrays
constexpr size_t CoreXYU_AXES = 5;									// The number of axes in a CoreXYU machine (there is a hidden V axis)
constexpr size_t CoreXYUV_AXES = 5;									// The number of axes in a CoreXYUV machine
constexpr size_t U_AXIS = 3, V_AXIS = 4;							// The indices of the U and V motors in a CoreXYU machine (needed by Platform)

// Common conversion factors
constexpr float MinutesToSeconds = 60.0;
constexpr float SecondsToMinutes = 1.0/MinutesToSeconds;
constexpr float SecondsToMillis = 1000.0;
constexpr float MillisToSeconds = 0.001;
constexpr float StepClocksToMillis = 1000.0/(float)StepClockRate;
constexpr float InchToMm = 25.4;
constexpr float Pi = 3.141592653589793;
constexpr float TwoPi = 3.141592653589793 * 2;
constexpr float DegreesToRadians = 3.141592653589793/180.0;
constexpr float RadiansToDegrees = 180.0/3.141592653589793;

#define DEGREE_SYMBOL	"\xC2\xB0"									// degree-symbol encoding in UTF8

// Type of an offset in a file
typedef uint32_t FilePosition;
const FilePosition noFilePosition = 0xFFFFFFFF;

// Interrupt priorities - must be chosen with care! 0 is the highest priority, 15 is the lowest.
#if SAM4E || SAME70
const uint32_t NvicPriorityWatchdog = 0;		// the secondary watchdog has the highest priority
#endif

const uint32_t NvicPriorityPanelDueUart = 1;	// UART is highest to avoid character loss (it has only a 1-character receive buffer)
const uint32_t NvicPriorityDriversSerialTMC = 2; // USART or UART used to control and monitor the smart drivers

#ifndef RTOS
const uint32_t NvicPrioritySystick = 3;			// systick kicks the watchdog and starts the ADC conversions, so must be quite high
#endif

const uint32_t NvicPriorityPins = 4;			// priority for GPIO pin interrupts - filament sensors must be higher than step

// Currently we set configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY in FreeRTOS to 5.
// This means that only interrupts with priority numerically at least 5 can make ISR-safe calls to FreeRTOS in an ISR.

const uint32_t NvicPriorityStep = 6;			// step interrupt is next highest, it can preempt most other interrupts
const uint32_t NvicPriorityWiFiUart = 7;		// UART used to receive debug data from the WiFi module
const uint32_t NvicPriorityUSB = 7;				// USB interrupt
const uint32_t NvicPriorityHSMCI = 7;			// HSMCI command complete interrupt

#if HAS_LWIP_NETWORKING
const uint32_t NvicPriorityNetworkTick = 8;		// priority for network tick interrupt
const uint32_t NvicPriorityEthernet = 8;		// priority for Ethernet interface
#endif

const uint32_t NvicPrioritySpi = 8;				// SPI is used for network transfers on Duet WiFi/Duet vEthernet
const uint32_t NvicPriorityTwi = 9;				// TWI is used to read endstop and other inputs on the DueXn

#endif
