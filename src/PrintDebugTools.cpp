#include <Errors.h>
#include <SupportDefs.h>
#include <GraphicsDefs.h>
#include <stdio.h>

void PrintGeneralError(status_t error);
void PrintKernelError(status_t error);
void PrintAppKitError(status_t error);
void PrintStorageKitError(status_t error);
void PrintPOSIXError(status_t error);
void PrintMediaKitError(status_t error);
void PrintMailKitError(status_t error);
void PrintDeviceKitError(status_t error);

void PrintSourceKit(status_t error)
{
	uint16 low_word=uint16(error & 0xF000);
	low_word >>= 8;
	switch(low_word)
	{
		case 0:
			printf("Source: General Error\n");
			break;
		case 0x10:
			printf("Source: Operating System\n");
			break;
		case 0x20:
			printf("Source: App Kit\n");
			break;
		case 0x30:
			printf("Source: Interface Kit\n");
			break;
		case 0x40:
			printf("Source: Media Kit\n");
			break;
		case 0x48:
			printf("Source: Translation Kit\n");
			break;
		case 0x50:
			printf("Source: MIDI Kit\n");
			break;
		case 0x60:
			printf("Source: Storage Kit\n");
			break;
		case 0x70:
			printf("Source: POSIX\n");
			break;
		case 0x80:
			printf("Source: Mail Kit\n");
			break;
		case 0x90:
			printf("Source: Print Kit\n");
			break;
		case 0xa0:
			printf("Source: Device Kit\n");
			break;
		default:
			printf("Source: Unknown - code 0x%x\n",low_word);
			break;
	}
}

void PrintErrorCode(status_t error)
{
	uint16 low_word=uint16(error & 0xF000);
	low_word >>= 8;
	switch(low_word)
	{
		case 0:
			PrintGeneralError(error);
			break;
		case 0x10:
			PrintKernelError(error);
			break;
		case 0x20:
			PrintAppKitError(error);
			break;
		case 0x40:
			PrintMediaKitError(error);
			break;
		case 0x60:
			PrintStorageKitError(error);
			break;
		case 0x70:
			PrintPOSIXError(error);
			break;
		case 0x80:
			PrintMailKitError(error);
			break;
		case 0xa0:
			PrintDeviceKitError(error);
			break;
		default:
			printf("Unknown error value 0x%lx\n",error);
			break;
	}
}

void PrintColorSpace(color_space value)
{
	switch(value)
	{
		case B_RGB32:
			printf("Color Space: B_RGB32\n");
			break;
		case B_RGBA32:
			printf("Color Space: B_RGBA32\n");
			break;
		case B_RGB32_BIG:
			printf("Color Space: B_RGB32_BIG\n");
			break;
		case B_RGBA32_BIG:
			printf("Color Space: \n");
			break;
		case B_UVL32:
			printf("Color Space: B_UVL32\n");
			break;
		case B_UVLA32:
			printf("Color Space: B_UVLA32\n");
			break;
		case B_LAB32:
			printf("Color Space: B_LAB32\n");
			break;
		case B_LABA32:
			printf("Color Space: B_LABA32\n");
			break;
		case B_HSI32:
			printf("Color Space: B_HSI32\n");
			break;
		case B_HSIA32:
			printf("Color Space: B_HSIA32\n");
			break;
		case B_HSV32:
			printf("Color Space: B_HSV32\n");
			break;
		case B_HSVA32:
			printf("Color Space: B_HSVA32\n");
			break;
		case B_HLS32:
			printf("Color Space: B_HLS32\n");
			break;
		case B_HLSA32:
			printf("Color Space: B_HLSA32\n");
			break;
		case B_CMY32:
			printf("Color Space: B_CMY32");
			break;
		case B_CMYA32:
			printf("Color Space: B_CMYA32\n");
			break;
		case B_CMYK32:
			printf("Color Space: B_CMYK32\n");
			break;
		case B_RGB24_BIG:
			printf("Color Space: B_RGB24_BIG\n");
			break;
		case B_RGB24:
			printf("Color Space: B_RGB24\n");
			break;
		case B_LAB24:
			printf("Color Space: B_LAB24\n");
			break;
		case B_UVL24:
			printf("Color Space: B_UVL24\n");
			break;
		case B_HSI24:
			printf("Color Space: B_HSI24\n");
			break;
		case B_HSV24:
			printf("Color Space: B_HSV24\n");
			break;
		case B_HLS24:
			printf("Color Space: B_HLS24\n");
			break;
		case B_CMY24:
			printf("Color Space: B_CMY24\n");
			break;
		case B_GRAY1:
			printf("Color Space: B_GRAY1\n");
			break;
		case B_CMAP8:
			printf("Color Space: B_CMAP8\n");
			break;
		case B_GRAY8:
			printf("Color Space: B_GRAY8\n");
			break;
		case B_YUV411:
			printf("Color Space: B_YUV411\n");
			break;
		case B_YUV420:
			printf("Color Space: B_YUV420\n");
			break;
		case B_YCbCr422:
			printf("Color Space: B_YCbCr422\n");
			break;
		case B_YCbCr411:
			printf("Color Space: B_YCbCr411\n");
			break;
		case B_YCbCr420:
			printf("Color Space: B_YCbCr420\n");
			break;
		case B_YUV422:
			printf("Color Space: B_YUV422\n");
			break;
		case B_YUV9:
			printf("Color Space: B_YUV9\n");
			break;
		case B_YUV12:
			printf("Color Space: B_YUV12\n");
			break;
		case B_RGB15:
			printf("Color Space: B_RGB15\n");
			break;
		case B_RGBA15:
			printf("Color Space: B_RGBA15\n");
			break;
		case B_RGB16:
			printf("Color Space: B_RGB16\n");
			break;
		case B_RGB16_BIG:
			printf("Color Space: B_RGB16_BIG\n");
			break;
		case B_RGB15_BIG:
			printf("Color Space: B_RGB15_BIG\n");
			break;
		case B_RGBA15_BIG:
			printf("Color Space: B_RGBA15_BIG\n");
			break;
		case B_YCbCr444:
			printf("Color Space: B_YCbCr444\n");
			break;
		case B_YUV444:
			printf("Color Space: B_YUV444\n");
			break;
		case B_NO_COLOR_SPACE:
			printf("Color Space: B_NO_COLOR_SPACE\n");
			break;
		default:
			printf("Color Space: Undefined color space\n");
			break;
	}
}

void PrintMessageCode(int32 code)
{
	// Used to translate BMessage message codes back to a character
	// format
		printf("Message code %c%c%c%c\n",
		(char)((code & 0xFF000000) >>  24),
		(char)((code & 0x00FF0000) >>  16),
		(char)((code & 0x0000FF00) >>  8),
		(char)((code & 0x000000FF)) );
}

void PrintGeneralError(status_t error)
{
	switch(error)
	{
		case B_NO_MEMORY:
			printf("B_NO_MEMORY\n");
			break;
		case B_IO_ERROR:
			printf("B_IO_ERROR\n");
			break;
		case B_PERMISSION_DENIED:
			printf("B_PERMISSION_DENIED\n");
			break;
		case B_BAD_INDEX:
			printf("B_BAD_INDEX\n");
			break;
		case B_BAD_TYPE:
			printf("B_BAD_TYPE\n");
			break;
		case B_BAD_VALUE:
			printf("B_BAD_VALUE\n");
			break;
		case B_MISMATCHED_VALUES:
			printf("B_MISMATCHED_VALUES\n");
			break;
		case B_NAME_NOT_FOUND:
			printf("B_NAME_NOT_FOUND\n");
			break;
		case B_NAME_IN_USE:
			printf("B_NAME_IN_USE\n");
			break;
		case B_TIMED_OUT:
			printf("B_TIMED_OUT\n");
			break;
		case B_INTERRUPTED:
			printf("B_INTERRUPTED\n");
			break;
		case B_WOULD_BLOCK:
			printf("B_WOULD_BLOCK\n");
			break;
		case B_CANCELED:
			printf("B_CANCELED\n");
			break;
		case B_NO_INIT:
			printf("B_NO_INIT\n");
			break;
		case B_BUSY:
			printf("B_BUSY\n");
			break;
		case B_NOT_ALLOWED:
			printf("B_NOT_ALLOWED\n");
			break;
		case B_OK:
			printf("B_OK\n");
			break;
			
		default:
			printf("Unknown error value 0x%lx\n",error);
			break;
	}
}

void PrintKernelError(status_t error)
{
	switch(error)
	{
		case B_BAD_SEM_ID:
			printf("B_BAD_SEM_ID\n");
			break;
		case B_NO_MORE_SEMS:
			printf("B_NO_MORE_SEMS\n");
			break;
		case B_BAD_THREAD_ID:
			printf("B_BAD_THREAD_ID\n");
			break;
		case B_BAD_THREAD_STATE:
			printf("B_BAD_THREAD_STATE\n");
			break;
		case B_BAD_TEAM_ID:
			printf("B_BAD_TEAM_ID\n");
			break;
		case B_NO_MORE_TEAMS:
			printf("B_NO_MORE_TEAMS\n");
			break;
		case B_BAD_PORT_ID:
			printf("B_BAD_PORT_ID\n");
			break;
		case B_NO_MORE_PORTS:
			printf("B_NO_MORE_PORTS\n");
			break;
		case B_BAD_IMAGE_ID:
			printf("B_BAD_IMAGE_ID\n");
			break;
		case B_BAD_ADDRESS:
			printf("B_BAD_ADDRESS\n");
			break;
		case B_NOT_AN_EXECUTABLE:
			printf("B_NOT_AN_EXECUTABLE\n");
			break;
		case B_MISSING_LIBRARY:
			printf("B_MISSING_LIBRARY\n");
			break;
		case B_MISSING_SYMBOL:
			printf("B_MISSING_SYMBOL\n");
			break;
		case B_DEBUGGER_ALREADY_INSTALLED:
			printf("B_DEBUGGER_ALREADY_INSTALLED\n");
			break;
		default:
			printf("Kernel Kit Error - unknown code 0x%lx\n",error);
			break;
	}
}

void PrintAppKitError(status_t error)
{
	switch(error)
	{
		case B_BAD_REPLY:
			printf("B_BAD_REPLY\n");
			break;
		case B_DUPLICATE_REPLY:
			printf("B_DUPLICATE_REPLY\n");
			break;
		case B_MESSAGE_TO_SELF:
			printf("B_MESSAGE_TO_SELF\n");
			break;
		case B_BAD_HANDLER:
			printf("B_BAD_HANDLER\n");
			break;
		case B_ALREADY_RUNNING:
			printf("B_ALREADY_RUNNING\n");
			break;
		case B_LAUNCH_FAILED:
			printf("B_LAUNCH_FAILED\n");
			break;
		case B_AMBIGUOUS_APP_LAUNCH:
			printf("B_AMBIGUOUS_APP_LAUNCH\n");
			break;
		case B_UNKNOWN_MIME_TYPE:
			printf("B_UNKNOWN_MIME_TYPE\n");
			break;
		case B_BAD_SCRIPT_SYNTAX:
			printf("B_BAD_SCRIPT_SYNTAX\n");
			break;
		case B_LAUNCH_FAILED_NO_RESOLVE_LINK:
			printf("B_LAUNCH_FAILED_NO_RESOLVE_LINK\n");
			break;
		case B_LAUNCH_FAILED_EXECUTABLE:
			printf("B_LAUNCH_FAILED_EXECUTABLE\n");
			break;
		case B_LAUNCH_FAILED_APP_NOT_FOUND:
			printf("B_LAUNCH_FAILED_APP_NOT_FOUND\n");
			break;
		case B_LAUNCH_FAILED_APP_IN_TRASH:
			printf("B_LAUNCH_FAILED_APP_IN_TRASH\n");
			break;
		case B_LAUNCH_FAILED_NO_PREFERRED_APP:
			printf("B_LAUNCH_FAILED_NO_PREFERRED_APP\n");
			break;
		case B_LAUNCH_FAILED_FILES_APP_NOT_FOUND:
			printf("B_LAUNCH_FAILED_FILES_APP_NOT_FOUND\n");
			break;
		case B_BAD_MIME_SNIFFER_RULE:
			printf("B_BAD_MIME_SNIFFER_RULE\n");
			break;
		default:
			printf("Application Kit Error - unknown code 0x%lx\n",error);
			break;
	}
}

void PrintStorageKitError(status_t error)
{
	switch(error)
	{
		case B_FILE_ERROR:
			printf("B_FILE_ERROR\n");
			break;
		case B_ENTRY_NOT_FOUND:
			printf("B_ENTRY_NOT_FOUND\n");
			break;
		case B_FILE_EXISTS:
			printf("B_FILE_EXISTS\n");
			break;
		case B_NAME_TOO_LONG:
			printf("B_NAME_TOO_LONG\n");
			break;
		case B_NOT_A_DIRECTORY:
			printf("B_NOT_A_DIRECTORY\n");
			break;
		case B_DIRECTORY_NOT_EMPTY:
			printf("B_DIRECTORY_NOT_EMPTY\n");
			break;
		case B_DEVICE_FULL:
			printf("B_DEVICE_FULL\n");
			break;
		case B_READ_ONLY_DEVICE:
			printf("B_READ_ONLY_DEVICE\n");
			break;
		case B_IS_A_DIRECTORY:
			printf("B_IS_A_DIRECTORY\n");
			break;
		case B_NO_MORE_FDS:
			printf("B_NO_MORE_FDS\n");
			break;
		case B_CROSS_DEVICE_LINK:
			printf("B_CROSS_DEVICE_LINK\n");
			break;
		case B_LINK_LIMIT:
			printf("B_LINK_LIMIT\n");
			break;
		case B_BUSTED_PIPE:
			printf("B_BUSTED_PIPE\n");
			break;
		case B_UNSUPPORTED:
			printf("B_UNSUPPORTED\n");
			break;
		case B_PARTITION_TOO_SMALL:
			printf("B_PARTITION_TOO_SMALL\n");
			break;
		default:
			printf("Storage Kit Error - unknown code 0x%lx\n",error);
			break;
	}
}

void PrintPOSIXError(status_t error)
{
	switch(error)
	{
		default:
			printf("POSIX Error - unknown code 0x%lx\n",error);
			break;
	}
}

void PrintMediaKitError(status_t error)
{
	switch(error)
	{
		default:
			printf("Media Kit Error - unknown code 0x%lx\n",error);
			break;
	}
}

void PrintMailKitError(status_t error)
{
	switch(error)
	{
		default:
			printf("Mail Kit Error - unknown code 0x%lx\n",error);
			break;
	}
}

void PrintDeviceKitError(status_t error)
{
	switch(error)
	{
		default:
			printf("Device Kit Error - unknown code 0x%lx\n",error);
			break;
	}
}
