#include "../include/stdlib.h"

/*unsigned long hex2int(char *a, unsigned int len)
{
    int i;
    unsigned long val = 0;

    for(i=0;i<len;i++)
        if(a[i] >= 48 && a[i] <= 57)
            val += (a[i]-48)*(1<<(4*(len-1-i)));
        else if(a[i] >= 65 && a[i] <= 70)
            val += (a[i]-55)*(1<<(4*(len-1-i)));
        else if(a[i] >= 97 && a[i] <= 102)
            val += (a[i]-87)*(1<<(4*(len-1-i)));
    return val;
}*/

int atoi(char *str)
{
    if (*str == '\0')
        return 0;

    int res = 0;		// Initialize result
    int sign = 1;		// Initialize sign as positive
    int i = 0;			// Initialize index of first digit

    if (str[0] == '-')
    {
        sign = -1;
        i++;  			// Also update index of first digit
    }

    for (; str[i] != '\0'; ++i)
    {
        if ( str[i] < '0' || str[i] > '9')	// If string contain character it will terminate
            return 0;

        res = res * 10 + str[i] - '0';
    }

    return sign * res;
}

char *itoa(int value, char *str, int base)
{
    char * rc;
    char * ptr;
    char * low;
    // Check for supported base.
    if ( base < 2 || base > 36 )
    {
        *str = '\0';
        return str;
    }
    rc = ptr = str;
    // Set '-' for negative decimals.
    if ( value < 0 && base == 10 )
    {
        *ptr++ = '-';
    }
    // Remember where the numbers start.
    low = ptr;
    // The actual conversion.
    do
    {
        // Modulo is negative for negative value. This trick makes abs() unnecessary.
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + value % base];
        value /= base;
    } while ( value );
    // Terminating the string.
    *ptr-- = '\0';
    // Invert the numbers.
    while ( low < ptr )
    {
        char tmp = *low;
        *low++ = *ptr;
        *ptr-- = tmp;
    }

    return rc;
}

char *utoa(uint32_t value, char *str, int base)
{
    char * rc;
    char * ptr;
    char * low;
    // Check for supported base.
    if ( base < 2 || base > 36 )
    {
        *str = '\0';
        return str;
    }
    rc = ptr = str;
    // Set '-' for negative decimals.
    if ( value < 0 && base == 10 )
    {
        *ptr++ = '-';
    }
    // Remember where the numbers start.
    low = ptr;
    // The actual conversion.
    do
    {
        // Modulo is negative for negative value. This trick makes abs() unnecessary.
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + value % base];
        value /= base;
    } while ( value );
    // Terminating the string.
    *ptr-- = '\0';
    // Invert the numbers.
    while ( low < ptr )
    {
        char tmp = *low;
        *low++ = *ptr;
        *ptr-- = tmp;
    }

    return rc;
}
