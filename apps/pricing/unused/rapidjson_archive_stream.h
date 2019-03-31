// archive_read_data_block is the "zero-copy variant" which sounds efficient.
// But it provides a pointer to the decompression buffer of p. Which it first
// needs to write the decompressed data into. If we call archive_read_data, we
// tell it to decompress a requested number of bytes into the user-provided
// buffer ... which could be just as efficient. It could even be more efficient
// because afterwards we can use the contiguous buffer without all the other
// checks.

// Also we can use in-situ parsing with rapidjson if we own the buffer.
//
// Experimental results: using rapidjson_archive_stream is much slower than the alternative. It looks like the compiler
// cannot optimize the hot paths in rapidjson properly because they contain the calls to archive_read_data_block, which
// act as optimization barriers.
//
class rapidjson_archive_stream {
  public:
    using Ch = uint8_t;

    rapidjson_archive_stream(::archive* p) : p_(p) { GetNextBlock(); }

    //! Read the current character from stream without moving the read cursor.
    Ch Peek() const { return data == stop ? '\0' : *data; }

    //! Read the current character from stream and moving the read cursor to next character.
    Ch Take() {
        auto c = *data++;
        if (data == stop)
            GetNextBlock();
        // return data == stop ? GetNextBlockAndTake() : *data++;
        return c;
    }

    //! Get the current read cursor.
    //! \return Number of characters read from start.
    size_t Tell() const {
        // we do it like this instead of increasing tell with every call to Take() because Take() gets called once per
        // character while Tell() only get's called to report error locations (And in ParseNumber it does startoffset =
        // Tell();
        // ...
        // length = Tell() - startoffset; )
        return tell - static_cast<size_t>(stop - data);
    }

    // these are required because rapidjson does not sfinae away the calls if they are not requested ...
    // But we don't need to provide implementations because after optimization the linker will never see them ...
    Ch* PutBegin();
    void Put(Ch c);
    void Flush();
    size_t PutEnd(Ch* begin);

  private:
    size_t GetNextBlock() {
        const void* start;
        size_t size;
        [[maybe_unused]] la_int64_t offset;
        auto status = archive_read_data_block(p_, &start, &size, &offset);

        // We do not throw if status == ARCHIVE_EOF because this just indicates that we have read the last block
        if (status < 0)
            throw std::runtime_error(archive_error_string(p_));

        // this catches out of bounds reads in Take() as well: If the file is
        // empty, the constructor calls GetNextBlock(), which will error out
        // here. If we call GetNextBlock() and get the last block (status ==
        // ARCHIVE_EOF), we will read until data == stop and we call
        // GetNextBlock() again. If archive_read_data_block returns anything
        // like an error code or size == 0, we will throw an exception.
        //
        // But archive_read_data_block does not specify a behavior if called at
        // EOF ...

        // std::cout << "read new block (" << size << " B): ";
        // std::cout.write(static_cast<const char*>(start), static_cast<int64_t>(size));
        // std::cout << '\n';

        data = static_cast<const Ch*>(start);
        stop = data + size;
        tell += size;
        return size;
    }

    Ch GetNextBlockAndTake() { return GetNextBlock() == 0 ? '\0' : *data++; }

    ::archive* p_;
    const Ch* data;
    const Ch* stop;
    size_t tell = 0;
};
