#pragma once

#define SAFE_BUFFER_DELETE(buf)		{ if (buf != NULL) { buffer_delete(buf); buf = NULL; } }

    typedef struct buffer	BUFFER;
    typedef struct buffer*	LPBUFFER;

    struct buffer
    {
		struct buffer* next;

		char*          write_point;
		int             write_point_pos;

		const char*    read_point;
		int             length;

		char*          mem_data;
		int             mem_size;

		long            flag;
		};

	extern LPBUFFER	buffer_new(int size);											// Create new buffer
    extern void		buffer_delete(LPBUFFER buffer);									// delete buffer
    extern void		buffer_reset(LPBUFFER buffer);									// Initialize buffer lengths

    extern uint32_t	buffer_size(LPBUFFER buffer);									// length remaining in buffer
    extern int		buffer_has_space(LPBUFFER buffer);								// Returns the usable length

    extern void		buffer_write (LPBUFFER& buffer, const void* src, int length);	// Write to the buffer
    extern void		buffer_read(LPBUFFER buffer, void* buf, int bytes);				// Read from buffer
    extern uint8_t		buffer_get_byte(LPBUFFER buffer);
    extern uint16_t		buffer_get_word(LPBUFFER buffer);
    extern uint32_t	buffer_get_dword(LPBUFFER buffer);

	// The buffer_proceed function needs to return a read pointer to buffer_peek and write it.
     // When present It is used when processing is finished and it is necessary to notify how much processing has been completed.
     // (In case of buffer_read, buffer_get_* series, it is handled by itself, but
     // because that can't be the case)
    extern const void*	buffer_read_peek(LPBUFFER buffer);					// Return the read position
    extern void		buffer_read_proceed(LPBUFFER buffer, int length);		// Processing of length is completed

	// Similarly, when getting a write position with write_peek and then reporting how much has been written
    // Use buffer_write_proceed.
    extern void*	buffer_write_peek(LPBUFFER buffer);						// Return the write position
    extern void		buffer_write_proceed(LPBUFFER buffer, int length);		// Only increase the length.

    extern void		buffer_adjust_size(LPBUFFER & buffer, int add_size);	// Securing the size to be added by add_size
