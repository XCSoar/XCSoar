# Screen Size Function Usage Analysis

## Summary

Analysis of screen size access patterns in Widgets, Forms, and Dialogs.

## Overall Assessment: **MOSTLY CONSISTENT** ✓

The codebase generally uses consistent patterns for accessing screen size, with a few minor inconsistencies.

## Patterns Found

### ✅ Standard Patterns (Correct Usage)

1. **`GetClientRect()`** - 43 occurrences
   - Most common pattern
   - Used by widgets and forms to get their own client area
   - **Status**: ✅ Correct and consistent

2. **`GetParentClientRect()`** - 2 occurrences
   - Used by dialogs to get parent window size
   - **Status**: ✅ Correct for dialogs
   - Files:
     - `src/Form/Form.cpp`
     - `src/Dialogs/WidgetDialog.cpp`

3. **`GetWindow().GetClientRect()`** - 6 occurrences (widgets)
   - Used by widgets to access their window's client rect
   - **Status**: ✅ Correct for widgets
   - Files:
     - `src/Widget/RowFormWidget.hpp`
     - `src/Widget/VScrollWidget.cpp`
     - `src/Widget/RowFormWidget.cpp`

4. **`main_window.GetClientRect()`** - 4 occurrences
   - Used by forms/dialogs to get main window size
   - **Status**: ✅ Correct for forms
   - Files:
     - `src/Form/Form.cpp`
     - `src/Dialogs/Message.cpp`
     - `src/Dialogs/LockScreen.cpp`

5. **`GetSize()`** - 36 occurrences
   - Window method to get size
   - **Status**: ✅ Correct (alternative to GetClientRect().GetSize())

### ⚠️ Potential Inconsistencies

The following files use **multiple different patterns**, which may indicate inconsistency:

1. **`src/Form/Form.cpp`**
   - Uses: `GetClientRect()`, `GetParentClientRect()`, `GetSize()`, `main_window.Get*()`
   - **Analysis**: This is the base Form class, so using multiple patterns may be intentional for different contexts

2. **`src/Dialogs/WidgetDialog.cpp`**
   - Uses: `GetClientRect()`, `GetParentClientRect()`, `GetSize()`
   - **Analysis**: Uses `GetParentClientRect()` for AutoSize() which is correct, but also uses other methods

3. **`src/Widget/RowFormWidget.cpp`**
   - Uses: `GetClientRect()`, `GetSize()`, `GetWindow().Get*()`
   - **Analysis**: Uses `window->GetSize().height` in some places, `GetWindow().GetClientRect()` in others
   - **Recommendation**: Consider standardizing to one pattern

4. **`src/Widget/RowFormWidget.hpp`**
   - Uses: `GetClientRect()`, `GetWindow().Get*()`
   - **Analysis**: Minor inconsistency between header and implementation

5. **`src/Widget/VScrollWidget.cpp`**
   - Uses: `GetClientRect()`, `GetWindow().Get*()`
   - **Analysis**: Uses both patterns, could be standardized

6. **`src/Widget/FixedWindowWidget.hpp`**
   - Uses: `GetSize()`, `GetWindow().Get*()`
   - **Analysis**: Uses `GetWindow().GetSize()` which is fine

7. **`src/Widget/ViewImageWidget.cpp`**
   - Uses: `GetClientRect()`, `GetSize()`
   - **Analysis**: Uses `GetClientRect()` and `rc.GetSize()` - both are fine

8. **`src/Form/TabDisplay.cpp`**
   - Uses: `GetClientRect()`, `GetSize()`
   - **Analysis**: Minor - both are valid

9. **`src/Dialogs/dlgCredits.cpp`**
   - Uses: `GetClientRect()`, `GetSize()`
   - **Analysis**: Minor - both are valid

10. **`src/Dialogs/Waypoint/dlgWaypointDetails.cpp`**
    - Uses: `GetClientRect()`, `GetSize()`
    - **Analysis**: Minor - both are valid

11. **`src/Dialogs/Message.cpp`**
    - Uses: `GetClientRect()`, `main_window.Get*()`
    - **Analysis**: Uses both - may be intentional for different contexts

12. **`src/Dialogs/KnobTextEntry.cpp`**
    - Uses: `GetClientRect()`, `GetWindow().Get*()`
    - **Analysis**: Uses both patterns

13. **`src/Dialogs/LockScreen.cpp`**
    - Uses: `GetClientRect()`, `GetSize()`, `main_window.Get*()`
    - **Analysis**: Uses multiple patterns

### ✅ Good News

- **No direct `SystemWindowSize()` usage** in widgets/forms/dialogs
- **No direct `display.GetSize()` usage** in widgets/forms/dialogs
- All screen size access goes through proper window methods

## Recommendations

### High Priority (Minor Inconsistencies)

1. **`src/Widget/RowFormWidget.cpp`** (lines 74, 104)
   - Currently uses: `window->GetSize().height`
   - Consider: `GetWindow().GetClientRect().GetHeight()` for consistency
   - **Impact**: Low - both work, but consistency is better

2. **`src/Widget/VScrollWidget.cpp`**
   - Uses both `GetClientRect()` and `GetWindow().GetClientRect()`
   - Consider standardizing to one pattern
   - **Impact**: Low

### Low Priority (Cosmetic)

Most other inconsistencies are minor and don't affect functionality. The patterns used are all valid, just not perfectly consistent.

## Standard Patterns to Follow

### For Widgets:
```cpp
// Preferred:
PixelRect rc = GetWindow().GetClientRect();
// or:
PixelRect rc = GetClientRect();  // if widget has direct access
```

### For Forms/Dialogs:
```cpp
// For own size:
PixelRect rc = GetClientRect();

// For parent/main window size:
PixelRect rc = GetParentClientRect();
// or:
PixelRect rc = main_window.GetClientRect();
```

### Avoid:
- ❌ `SystemWindowSize()` - use window methods instead
- ❌ `display.GetSize()` - use window methods instead
- ❌ Direct platform-specific calls

## Conclusion

The codebase is **mostly consistent** in screen size access. The inconsistencies found are minor and don't affect functionality. All patterns used are valid window methods. The main recommendation is to standardize within individual files for better code clarity, but this is a low-priority cleanup task.

**Overall Grade: B+** (Good consistency with minor room for improvement)



