/*************************************************************************
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * Copyright 2000, 2010 Oracle and/or its affiliates.
 *
 * OpenOffice.org - a multi-platform office productivity suite
 *
 * This file is part of OpenOffice.org.
 *
 * OpenOffice.org is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3
 * only, as published by the Free Software Foundation.
 *
 * OpenOffice.org is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License version 3 for more details
 * (a copy is included in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU Lesser General Public License
 * version 3 along with OpenOffice.org.  If not, see
 * <http://www.openoffice.org/license.html>
 * for a copy of the LGPLv3 License.
 *
 ************************************************************************/

#ifndef COMPHELPER_DOCPASSWORDHELPR_HXX
#define COMPHELPER_DOCPASSWORDHELPR_HXX

#include "comphelper/comphelperdllapi.h"
#include <vector>
#include "comphelper/docpasswordrequest.hxx"

namespace com { namespace sun { namespace star { namespace task { class XInteractionHandler; } } } }

namespace comphelper {

class MediaDescriptor;

// ============================================================================

enum DocPasswordVerifierResult
{
    DocPasswordVerifierResult_OK,
    DocPasswordVerifierResult_WRONG_PASSWORD,
    DocPasswordVerifierResult_ABORT
};

// ============================================================================

/** Base class for a password verifier used by the DocPasswordHelper class
    below.

    Users have to implement the virtual function and pass an instance of the
    verifier to one of the password request functions.
 */
class COMPHELPER_DLLPUBLIC IDocPasswordVerifier
{
public:
    virtual             ~IDocPasswordVerifier();

    /** Will be called everytime a password needs to be verified.

        @return  The result of the verification.
            - DocPasswordVerifierResult_OK, if and only if the passed password
              is valid and can be used to process the related document.
            - DocPasswordVerifierResult_WRONG_PASSWORD, if the password is
              wrong. The user may be asked again for a new password.
            - DocPasswordVerifierResult_ABORT, if an unrecoverable error
              occured while password verification. The password request loop
              will be aborted.
     */
    virtual DocPasswordVerifierResult verifyPassword( const ::rtl::OUString& rPassword ) = 0;

};

// ============================================================================

/** Helper that asks for a document password and checks its validity.
 */
class COMPHELPER_DLLPUBLIC DocPasswordHelper
{
public:
    // ------------------------------------------------------------------------

    /** This helper function tries to request and verify a password to load a
        protected document.

        First, the list of default passwords will be tried if provided. This is
        needed by import filters for external file formats that have to check a
        predefined password in some cases without asking the user for a
        password. Every password is checked using the passed password verifier.

        If not successful, the passed password of a medium is tried, that has
        been set e.g. by an API call to load a document. If existing, the
        password is checked using the passed password verifier.

        If still not successful, the passed interaction handler is used to
        request a password from the user. This will be repeated until the
        passed password verifier validates the entered password, or if the user
        chooses to cancel password input.

        @param rVerifier
            The password verifier used to check every processed password.

        @param rMediaPassword
            If not empty, will be passed to the password validator before
            requesting a password from the user. This password usually should
            be querried from a media descriptor.

        @param rxInteractHandler
            The interaction handler that will be used to request a password
            from the user, e.g. by showing a password input dialog.

        @param rDocumentName
            The name of the related document that will be shown in the password
            input dialog.

        @param eRequestType
            The password request type that will be passed to the
            DocPasswordRequest object created internally. See
            docpasswordrequest.hxx for more details.

        @param pDefaultPasswords
            If not null, contains default passwords that will be tried before a
            password will be requested from the media descriptor or the user.

        @param pbIsDefaultPassword
            (output parameter) If not null, the type of the found password will
            be returned. True means the password has been found in the passed
            list of default passwords. False means the password has been taken
            from the rMediaPassword parameter or has been entered by the user.

        @return
            If not empty, contains the password that has been validated by the
            passed password verifier. If empty, no valid password has been
            found, or the user has chossen to cancel password input.
     */
    static ::rtl::OUString requestAndVerifyDocPassword(
                            IDocPasswordVerifier& rVerifier,
                            const ::rtl::OUString& rMediaPassword,
                            const ::com::sun::star::uno::Reference<
                                ::com::sun::star::task::XInteractionHandler >& rxInteractHandler,
                            const ::rtl::OUString& rDocumentName,
                            DocPasswordRequestType eRequestType,
                            const ::std::vector< ::rtl::OUString >* pDefaultPasswords = 0,
                            bool* pbIsDefaultPassword = 0 );

    // ------------------------------------------------------------------------

    /** This helper function tries to find a password for the document
        described by the passed media descriptor.

        First, the list of default passwords will be tried if provided. This is
        needed by import filters for external file formats that have to check a
        predefined password in some cases without asking the user for a
        password. Every password is checked using the passed password verifier.

        If not successful, the passed media descriptor is asked for a password,
        that has been set e.g. by an API call to load a document. If existing,
        the password is checked using the passed password verifier.

        If still not successful, the interaction handler contained in the
        passed nmedia descriptor is used to request a password from the user.
        This will be repeated until the passed password verifier validates the
        entered password, or if the user chooses to cancel password input.

        @param rVerifier
            The password verifier used to check every processed password.

        @param rMediaDesc
            The media descriptor of the document that needs to be opened with
            a password. If a valid password (that is not contained in the
            passed list of default passwords) was found, it will be inserted
            into the "Password" property of this descriptor.

        @param eRequestType
            The password request type that will be passed to the
            DocPasswordRequest object created internally. See
            docpasswordrequest.hxx for more details.

        @param pDefaultPasswords
            If not null, contains default passwords that will be tried before a
            password will be requested from the media descriptor or the user.

        @return
            If not empty, contains the password that has been validated by the
            passed password verifier. If empty, no valid password has been
            found, or the user has chossen to cancel password input.
     */
    static ::rtl::OUString requestAndVerifyDocPassword(
                            IDocPasswordVerifier& rVerifier,
                            MediaDescriptor& rMediaDesc,
                            DocPasswordRequestType eRequestType,
                            const ::std::vector< ::rtl::OUString >* pDefaultPasswords = 0 );

    // ------------------------------------------------------------------------

private:
                        ~DocPasswordHelper();
};

// ============================================================================

} // namespace comphelper

#endif

