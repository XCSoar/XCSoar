#ifndef NONCOPYABLE_HPP
#define NONCOPYABLE_HPP

/**
 * Prevent object from being copied directly
 */
class NonCopyable
{
  protected:
    NonCopyable () {}
    ~NonCopyable () {} /// Protected non-virtual destructor
  private: 
    NonCopyable (const NonCopyable &);
    NonCopyable & operator = (const NonCopyable &);
};

#endif
