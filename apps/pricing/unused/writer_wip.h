// WriterImpl attempt with problem: we write to f_ "pages" of the same type in arbitrary order :(
// need "proper file format"

// template <typename T>
// class WriterProxy {
//  public:
//    WriterProxy(std::ofstream& f) : f_(&f) {}
//
//    // overwrite for types with expensive copy or non-pod types
//    // TODO: static assert ... how to check "expensive copy"?
//    void write(const T x) {
//        if (current + sizeof(T) > buffer.end()) {
//            f_->write(buffer.begin(), current - buffer.begin());
//            current = buffer.begin();
//        }
//        std::memcpy(current, &x, sizeof(T));
//        current += sizeof(T);
//    }
//
//  private:
//    std::array<char, 8192> buffer;
//    std::array<char, 8192>::iterator current{buffer.begin()};
//    std::ofstream* f_{nullptr};
//};

// template <typename... T>
// class WriterImpl {
//  public:
//    template <size_t... I>
//    WriterImpl(const char* file_name, std::index_sequence<I...>) : f_(file_name), writers{((void)I, f_)...} {}
//    WriterImpl(const char* file_name) : WriterImpl(file_name, std::make_index_sequence<sizeof...(T)>{}) {}
//    void write(const std::tuple<T...>&);
//
//  private:
//    std::ofstream f_;
//    std::tuple<WriterProxy<T>...> writers;
//};

// template <typename... T>
// void WriterImpl<T...>::write(const std::tuple<T...>& x) {
//    (std::get<WriterProxy<T>>(writers).write(std::get<T>(x)), ...);
//}

// Attempt with proper format: write std::tuple<T...> in order, one after the other
// Problem: We want to write "buy" and "range" as bits in one int, so we can't just write members independently one
// ofter the other

template <typename... T>
class WriterImpl {
  public:
    WriterImpl(const char* file_name) : f_(file_name) {}
    void write(const std::tuple<T...>&);

  private:
    std::ofstream f_;
};

template <typename... T>
void WriterImpl<T...>::write(const std::tuple<T...>& x) {
    write_tuple(f_, x);
}

template <typename T>
struct Writer_type;

template <typename... T>
struct Writer_type<std::tuple<T...>> {
    using type = WriterImpl<T...>;
};

template <typename T>
using Writer = typename Writer_type<T>::type;
