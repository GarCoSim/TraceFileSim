#ifndef TRACEFILESIM_OPTIONAL_H
#define TRACEFILESIM_OPTIONAL_H

/* We use optionals for all
 * these arguments since not
 * all of them have good
 * default values. Given an
 * Optional o:
 *
 * Where good defaults make sense use:
 *      o.getOr(default);
 *
 * Where good defaults don't make sense
 * and nonexistence is a fatal error use:
 *      o.getValue();
 *
 * Where good defaults don't make sense
 * and nonexistence isn't an error, pass
 * the optional as a parameter and use the
 * following in the function:
 *      if(o.isSet()){
 *          T v = o.getValue();
 *          // write code logic
 *      }
 */

template <typename T>
class Optional {
/* Fields */
public:
private:
    bool exists;
    T value;
/* Methods */
public:
    T getValue();
    bool isSet();
    void setValue(T value);
    T getOr(T def);
    Optional();
    Optional(T value);
private:
};


#endif //TRACEFILESIM_OPTIONAL_H
