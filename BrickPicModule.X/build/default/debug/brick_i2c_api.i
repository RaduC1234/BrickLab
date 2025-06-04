# 1 "brick_i2c_api.c"
# 1 "<built-in>" 1
# 1 "<built-in>" 3
# 285 "<built-in>" 3
# 1 "<command line>" 1
# 1 "<built-in>" 2
# 1 "C:\\Program Files\\Microchip\\pic\\include/language_support.h" 1 3
# 2 "<built-in>" 2
# 1 "brick_i2c_api.c" 2
# 1 "./brick_i2c_api.h" 1
# 16 "./brick_i2c_api.h"
# 1 "C:\\Program Files\\Microchip\\pic\\include\\c99/stdint.h" 1 3



# 1 "C:\\Program Files\\Microchip\\pic\\include\\c99/musl_xc8.h" 1 3
# 5 "C:\\Program Files\\Microchip\\pic\\include\\c99/stdint.h" 2 3
# 26 "C:\\Program Files\\Microchip\\pic\\include\\c99/stdint.h" 3
# 1 "C:\\Program Files\\Microchip\\pic\\include\\c99/bits/alltypes.h" 1 3
# 133 "C:\\Program Files\\Microchip\\pic\\include\\c99/bits/alltypes.h" 3
typedef unsigned __int24 uintptr_t;
# 148 "C:\\Program Files\\Microchip\\pic\\include\\c99/bits/alltypes.h" 3
typedef __int24 intptr_t;
# 164 "C:\\Program Files\\Microchip\\pic\\include\\c99/bits/alltypes.h" 3
typedef signed char int8_t;




typedef short int16_t;




typedef __int24 int24_t;




typedef long int32_t;





typedef long long int64_t;
# 194 "C:\\Program Files\\Microchip\\pic\\include\\c99/bits/alltypes.h" 3
typedef long long intmax_t;





typedef unsigned char uint8_t;




typedef unsigned short uint16_t;




typedef __uint24 uint24_t;




typedef unsigned long uint32_t;





typedef unsigned long long uint64_t;
# 235 "C:\\Program Files\\Microchip\\pic\\include\\c99/bits/alltypes.h" 3
typedef unsigned long long uintmax_t;
# 27 "C:\\Program Files\\Microchip\\pic\\include\\c99/stdint.h" 2 3

typedef int8_t int_fast8_t;

typedef int64_t int_fast64_t;


typedef int8_t int_least8_t;
typedef int16_t int_least16_t;

typedef int24_t int_least24_t;
typedef int24_t int_fast24_t;

typedef int32_t int_least32_t;

typedef int64_t int_least64_t;


typedef uint8_t uint_fast8_t;

typedef uint64_t uint_fast64_t;


typedef uint8_t uint_least8_t;
typedef uint16_t uint_least16_t;

typedef uint24_t uint_least24_t;
typedef uint24_t uint_fast24_t;

typedef uint32_t uint_least32_t;

typedef uint64_t uint_least64_t;
# 148 "C:\\Program Files\\Microchip\\pic\\include\\c99/stdint.h" 3
# 1 "C:\\Program Files\\Microchip\\pic\\include\\c99/bits/stdint.h" 1 3
typedef int16_t int_fast16_t;
typedef int32_t int_fast32_t;
typedef uint16_t uint_fast16_t;
typedef uint32_t uint_fast32_t;
# 149 "C:\\Program Files\\Microchip\\pic\\include\\c99/stdint.h" 2 3
# 17 "./brick_i2c_api.h" 2
# 1 "C:\\Program Files\\Microchip\\pic\\include\\c99/stdbool.h" 1 3
# 18 "./brick_i2c_api.h" 2
# 30 "./brick_i2c_api.h"
typedef enum {
    CMD_IDENTIFY = 0x00,
    CMD_LED = 0x01,
    CMD_LED_DOUBLE = 0x02,
    CMD_LED_RGB = 0x03,
    CMD_SERVO_SET_ANGLE = 0x10,
    CMD_STEPPER_MOVE = 0x11,
    CMD_SENSOR_GET_CM = 0x30
} brick_command_type_t;
# 48 "./brick_i2c_api.h"
typedef enum {
    BRICK_TYPE_LED_BASE = 0x1000,
    BRICK_TYPE_MOTOR_BASE = 0x2000,
    BRICK_TYPE_SENSOR_BASE = 0x3000
} brick_device_type_group_t;





typedef enum {
    LED_SINGLE = BRICK_TYPE_LED_BASE + 0x00,
    LED_DOUBLE = BRICK_TYPE_LED_BASE + 0x01,
    LED_RGB = BRICK_TYPE_LED_BASE + 0x10,

    MOTOR_SERVO_180 = BRICK_TYPE_MOTOR_BASE + 0x00,
    MOTOR_SERVO_360 = BRICK_TYPE_MOTOR_BASE + 0x01,
    MOTOR_STEPPER = BRICK_TYPE_MOTOR_BASE + 0x02,

    SENSOR_COLOR = BRICK_TYPE_SENSOR_BASE + 0x00,
    SENSOR_DISTANCE = BRICK_TYPE_SENSOR_BASE + 0x01
} brick_device_type_t;
# 79 "./brick_i2c_api.h"
typedef struct __attribute__((packed)) {
    uint8_t prefix[2];
    uint8_t device_type[2];
    uint8_t reserved[4];
    uint8_t unique_id[8];
} brick_uuid_raw_t;





typedef union {
    brick_uuid_raw_t raw;
    uint8_t bytes[16];
} brick_uuid_t;
# 102 "./brick_i2c_api.h"
typedef struct {
    uint8_t is_on;
} brick_device_led_single_impl_t;




typedef struct {
    uint8_t is_on_1;
    uint8_t is_on_2;
} brick_device_led_double_impl_t;




typedef struct {
    uint8_t red;
    uint8_t blue;
    uint8_t green;
} brick_device_led_rgb_impl_t;

typedef struct {
    uint8_t angle;
} brick_device_servo_180_impl_t;

typedef struct {
    uint8_t instr;
} brick_device_stepper_motor_impl_t;





typedef union {
    brick_device_led_single_impl_t led_single;
    brick_device_led_double_impl_t led_double;
    brick_device_led_rgb_impl_t led_rgb;
    brick_device_servo_180_impl_t servo_180;
    brick_device_stepper_motor_impl_t stepper_motor;
} brick_device_impl_t;
# 151 "./brick_i2c_api.h"
typedef struct {
    brick_uuid_t uuid;
    brick_device_type_t device_type;
    uint8_t i2c_address;
    brick_device_impl_t impl;
    uint8_t online;
} brick_device_t;





typedef struct {
    brick_command_type_t command;
    brick_device_t *device;
} brick_command_t;
# 177 "./brick_i2c_api.h"
brick_device_t brick_get_device_specs_from_uuid(const uint8_t *uuid);






uint8_t brick_uuid_get_i2c_address(const brick_uuid_t *uuid);






_Bool brick_uuid_valid(const uint8_t *uuid);





void brick_print_uuid(const brick_uuid_t *uuid);






const char *brick_device_type_str(brick_device_type_t type);
# 2 "brick_i2c_api.c" 2





# 1 "C:\\Program Files\\Microchip\\pic\\include\\c99/stdio.h" 1 3
# 10 "C:\\Program Files\\Microchip\\pic\\include\\c99/stdio.h" 3
# 1 "C:\\Program Files\\Microchip\\pic\\include\\c99/features.h" 1 3
# 11 "C:\\Program Files\\Microchip\\pic\\include\\c99/stdio.h" 2 3
# 24 "C:\\Program Files\\Microchip\\pic\\include\\c99/stdio.h" 3
# 1 "C:\\Program Files\\Microchip\\pic\\include\\c99/bits/alltypes.h" 1 3
# 12 "C:\\Program Files\\Microchip\\pic\\include\\c99/bits/alltypes.h" 3
typedef void * va_list[1];




typedef void * __isoc_va_list[1];
# 128 "C:\\Program Files\\Microchip\\pic\\include\\c99/bits/alltypes.h" 3
typedef unsigned size_t;
# 143 "C:\\Program Files\\Microchip\\pic\\include\\c99/bits/alltypes.h" 3
typedef __int24 ssize_t;
# 255 "C:\\Program Files\\Microchip\\pic\\include\\c99/bits/alltypes.h" 3
typedef long long off_t;
# 409 "C:\\Program Files\\Microchip\\pic\\include\\c99/bits/alltypes.h" 3
typedef struct _IO_FILE FILE;
# 25 "C:\\Program Files\\Microchip\\pic\\include\\c99/stdio.h" 2 3
# 52 "C:\\Program Files\\Microchip\\pic\\include\\c99/stdio.h" 3
typedef union _G_fpos64_t {
 char __opaque[16];
 double __align;
} fpos_t;

extern FILE *const stdin;
extern FILE *const stdout;
extern FILE *const stderr;





FILE *fopen(const char *restrict, const char *restrict);
FILE *freopen(const char *restrict, const char *restrict, FILE *restrict);
int fclose(FILE *);

int remove(const char *);
int rename(const char *, const char *);

int feof(FILE *);
int ferror(FILE *);
int fflush(FILE *);
void clearerr(FILE *);

int fseek(FILE *, long, int);
long ftell(FILE *);
void rewind(FILE *);

int fgetpos(FILE *restrict, fpos_t *restrict);
int fsetpos(FILE *, const fpos_t *);

size_t fread(void *restrict, size_t, size_t, FILE *restrict);
size_t fwrite(const void *restrict, size_t, size_t, FILE *restrict);

int fgetc(FILE *);
int getc(FILE *);
int getchar(void);





int ungetc(int, FILE *);
int getch(void);

int fputc(int, FILE *);
int putc(int, FILE *);
int putchar(int);





void putch(char);

char *fgets(char *restrict, int, FILE *restrict);

char *gets(char *);


int fputs(const char *restrict, FILE *restrict);
int puts(const char *);

__attribute__((__format__(__printf__, 1, 2)))
int printf(const char *restrict, ...);
__attribute__((__format__(__printf__, 2, 3)))
int fprintf(FILE *restrict, const char *restrict, ...);
__attribute__((__format__(__printf__, 2, 3)))
int sprintf(char *restrict, const char *restrict, ...);
__attribute__((__format__(__printf__, 3, 4)))
int snprintf(char *restrict, size_t, const char *restrict, ...);

__attribute__((__format__(__printf__, 1, 0)))
int vprintf(const char *restrict, __isoc_va_list);
int vfprintf(FILE *restrict, const char *restrict, __isoc_va_list);
__attribute__((__format__(__printf__, 2, 0)))
int vsprintf(char *restrict, const char *restrict, __isoc_va_list);
__attribute__((__format__(__printf__, 3, 0)))
int vsnprintf(char *restrict, size_t, const char *restrict, __isoc_va_list);

__attribute__((__format__(__scanf__, 1, 2)))
int scanf(const char *restrict, ...);
__attribute__((__format__(__scanf__, 2, 3)))
int fscanf(FILE *restrict, const char *restrict, ...);
__attribute__((__format__(__scanf__, 2, 3)))
int sscanf(const char *restrict, const char *restrict, ...);

__attribute__((__format__(__scanf__, 1, 0)))
int vscanf(const char *restrict, __isoc_va_list);
int vfscanf(FILE *restrict, const char *restrict, __isoc_va_list);
__attribute__((__format__(__scanf__, 2, 0)))
int vsscanf(const char *restrict, const char *restrict, __isoc_va_list);

void perror(const char *);

int setvbuf(FILE *restrict, char *restrict, int, size_t);
void setbuf(FILE *restrict, char *restrict);

char *tmpnam(char *);
FILE *tmpfile(void);




FILE *fmemopen(void *restrict, size_t, const char *restrict);
FILE *open_memstream(char **, size_t *);
FILE *fdopen(int, const char *);
FILE *popen(const char *, const char *);
int pclose(FILE *);
int fileno(FILE *);
int fseeko(FILE *, off_t, int);
off_t ftello(FILE *);
int dprintf(int, const char *restrict, ...);
int vdprintf(int, const char *restrict, __isoc_va_list);
void flockfile(FILE *);
int ftrylockfile(FILE *);
void funlockfile(FILE *);
int getc_unlocked(FILE *);
int getchar_unlocked(void);
int putc_unlocked(int, FILE *);
int putchar_unlocked(int);
ssize_t getdelim(char **restrict, size_t *restrict, int, FILE *restrict);
ssize_t getline(char **restrict, size_t *restrict, FILE *restrict);
int renameat(int, const char *, int, const char *);
char *ctermid(char *);







char *tempnam(const char *, const char *);
# 8 "brick_i2c_api.c" 2
# 1 "C:\\Program Files\\Microchip\\pic\\include\\c99/string.h" 1 3
# 25 "C:\\Program Files\\Microchip\\pic\\include\\c99/string.h" 3
# 1 "C:\\Program Files\\Microchip\\pic\\include\\c99/bits/alltypes.h" 1 3
# 421 "C:\\Program Files\\Microchip\\pic\\include\\c99/bits/alltypes.h" 3
typedef struct __locale_struct * locale_t;
# 26 "C:\\Program Files\\Microchip\\pic\\include\\c99/string.h" 2 3

void *memcpy (void *restrict, const void *restrict, size_t);
void *memmove (void *, const void *, size_t);
void *memset (void *, int, size_t);
int memcmp (const void *, const void *, size_t);
void *memchr (const void *, int, size_t);

char *strcpy (char *restrict, const char *restrict);
char *strncpy (char *restrict, const char *restrict, size_t);

char *strcat (char *restrict, const char *restrict);
char *strncat (char *restrict, const char *restrict, size_t);

int strcmp (const char *, const char *);
int strncmp (const char *, const char *, size_t);

int strcoll (const char *, const char *);
size_t strxfrm (char *restrict, const char *restrict, size_t);

char *strchr (const char *, int);
char *strrchr (const char *, int);

size_t strcspn (const char *, const char *);
size_t strspn (const char *, const char *);
char *strpbrk (const char *, const char *);
char *strstr (const char *, const char *);
char *strtok (char *restrict, const char *restrict);

size_t strlen (const char *);

char *strerror (int);




char *strtok_r (char *restrict, const char *restrict, char **restrict);
int strerror_r (int, char *, size_t);
char *stpcpy(char *restrict, const char *restrict);
char *stpncpy(char *restrict, const char *restrict, size_t);
size_t strnlen (const char *, size_t);
char *strdup (const char *);
char *strndup (const char *, size_t);
char *strsignal(int);
char *strerror_l (int, locale_t);
int strcoll_l (const char *, const char *, locale_t);
size_t strxfrm_l (char *restrict, const char *restrict, size_t, locale_t);




void *memccpy (void *restrict, const void *restrict, int, size_t);
# 9 "brick_i2c_api.c" 2

brick_device_t brick_get_device_specs_from_uuid(const uint8_t *uuid_bytes) {
    brick_device_t device = {0};

    if (!uuid_bytes) return device;

    memcpy(device.uuid.bytes, uuid_bytes, 16);

    uint16_t type = (uuid_bytes[2] << 8) | uuid_bytes[3];
    device.device_type = (brick_device_type_t) type;

    device.i2c_address = brick_uuid_get_i2c_address(&device.uuid);
    device.online = 0;

    return device;
}

uint8_t brick_uuid_get_i2c_address(const brick_uuid_t *uuid) {
    if (!uuid)
        return 0x00;

    const uint8_t raw = uuid->raw.unique_id[0];
    return 0x08 + raw % (0x78 - 0x08);
}

_Bool brick_uuid_valid(const uint8_t *uuid) {
    return uuid && uuid[0] == 'B' && uuid[1] == 'L';
}

void brick_print_uuid(const brick_uuid_t *uuid) {
    printf("UUID: ");
    for (int i = 0; i < 16; ++i)
        printf("%02X ", uuid->bytes[i]);
    printf("\n");
}

const char *brick_device_type_str(brick_device_type_t type) {
    switch (type) {
        case LED_SINGLE: return "LED_SINGLE";
        case LED_DOUBLE: return "LED_DOUBLE";
        case LED_RGB: return "LED_RGB";
        case MOTOR_SERVO_180: return "MOTOR_SERVO_180";
        case MOTOR_SERVO_360: return "MOTOR_SERVO_360";
        case MOTOR_STEPPER: return "MOTOR_STEPPER";
        case SENSOR_COLOR: return "SENSOR_COLOR";
        case SENSOR_DISTANCE: return "SENSOR_DISTANCE";
        default: return "UNKNOWN";
    }
}

void brick_load_and_run(const char *program_source) {
}

void brick_get_host_modules(void) {
}
