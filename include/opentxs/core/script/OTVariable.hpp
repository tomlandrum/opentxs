// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_SCRIPT_OTVARIABLE_HPP
#define OPENTXS_CORE_SCRIPT_OTVARIABLE_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/core/String.hpp"

#include <cstdint>
#include <string>

namespace opentxs
{

class OTBylaw;
class OTScript;
class OTVariable;
class Tag;

class OTVariable
{
public:
    enum OTVariable_Type {
        Var_String,   // std::string
        Var_Integer,  // Integer. (For std::int64_t std::int32_t: use strings.)
        Var_Bool,     // Boolean. (True / False)
        Var_Error_Type  // should never happen.
    };

    enum OTVariable_Access {
        Var_Constant,     // Constant   -- you cannot change this value.
        Var_Persistent,   // Persistent -- changing value doesn't require notice
                          // to parties.
        Var_Important,    // Important  -- changing value requires notice to
                          // parties.
        Var_Error_Access  // should never happen.
    };

private:
    OTString m_strName;             // Name of this variable.
    std::string m_str_Value;        // If a string, the value is stored here.
    std::int32_t m_nValue{};        // If an integer, the value is stored here.
    bool m_bValue{false};           // If a bool, the value is stored here.
    std::string m_str_ValueBackup;  // If a string, the value backup is stored
                                    // here. (So we can see if it has changed
                                    // since execution)
    std::int32_t m_nValueBackup{};  // If an integer, the value backup is stored
                                    // here.
    // (So we can see if it has changed since execution)
    bool m_bValueBackup{false};  // If a bool, the value backup is stored here.
                                 // (So we can check for dirtiness later...)
    OTBylaw* m_pBylaw{nullptr};  // the Bylaw that this variable belongs to.
    OTVariable_Type m_Type{Var_Error_Type};  // Currently bool, std::int32_t, or
                                             // string.
    OTVariable_Access m_Access{Var_Error_Access};  // Determines how the
                                                   // variable is used inside
                                                   // the script.
    OTScript* m_pScript{nullptr};  // If the variable is set onto a script, this
                                   // pointer gets set. When the variable
                                   // destructs, it will remove itself from the
                                   // script.

public:
    EXPORT void RegisterForExecution(OTScript& theScript);  // We keep an
                                                            // internal script
                                                            // pointer here, so
    // if we destruct, we
    // can remove
    // ourselves from the
    // script.
    EXPORT void UnregisterScript();  // If the script destructs before the
                                     // variable does, it unregisters itself
                                     // here, so the variable isn't stuck with a
                                     // bad pointer.
    bool IsDirty() const;  // So you can tell if the variable has CHANGED since
                           // it was last set clean.
    void SetAsClean();  // Sets the variable as clean, so you can check it later
                        // and see if it's been changed (if it's DIRTY again.)
    bool IsConstant() const { return (Var_Constant == m_Access); }
    bool IsPersistent() const
    {
        return ((Var_Persistent == m_Access) || (Var_Important == m_Access));
    }  // important vars are persistent, too.
    bool IsImportant() const { return (Var_Important == m_Access); }
    void SetBylaw(OTBylaw& theBylaw) { m_pBylaw = &theBylaw; }
    bool SetValue(const std::int32_t& nValue);
    bool SetValue(bool bValue);
    bool SetValue(const std::string& str_Value);

    EXPORT const String& GetName() const
    {
        return m_strName;
    }  // variable's name as used in a script.
    OTVariable_Type GetType() const { return m_Type; }
    OTVariable_Access GetAccess() const { return m_Access; }

    bool IsInteger() const { return (Var_Integer == m_Type); }
    bool IsBool() const { return (Var_Bool == m_Type); }
    bool IsString() const { return (Var_String == m_Type); }

    std::int32_t CopyValueInteger() const { return m_nValue; }
    bool CopyValueBool() const { return m_bValue; }
    std::string CopyValueString() const { return m_str_Value; }

    std::int32_t& GetValueInteger() { return m_nValue; }
    bool& GetValueBool() { return m_bValue; }
    std::string& GetValueString() { return m_str_Value; }

    bool Compare(OTVariable& rhs);

    EXPORT OTVariable();
    EXPORT OTVariable(
        const std::string& str_Name,
        const std::int32_t nValue,
        const OTVariable_Access theAccess = Var_Persistent);
    EXPORT OTVariable(
        const std::string& str_Name,
        const bool bValue,
        const OTVariable_Access theAccess = Var_Persistent);
    EXPORT OTVariable(
        const std::string& str_Name,
        const std::string& str_Value,
        const OTVariable_Access theAccess = Var_Persistent);
    EXPORT virtual ~OTVariable();

    void Serialize(Tag& parent, bool bCalculatingID = false) const;
};

}  // namespace opentxs

#endif
