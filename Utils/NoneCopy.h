#ifndef NONECOPY_H
#define NONECOPY_H

class NoneCopy
{
public:
    ~NoneCopy() = default;
protected:
    NoneCopy() = default;
private:
    NoneCopy(const NoneCopy&) = delete;
    NoneCopy& operator=(const NoneCopy&) = delete;
};

#endif // NONECOPY_H
