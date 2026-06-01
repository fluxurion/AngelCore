/*
 * RegionwideCharacterHandlers.as
 * Handlers for regionwide character data (restrictions, mail, etc.)
 *
 * Note: CMSG_GET_REGIONWIDE_CHARACTER_RESTRICTION_AND_MAIL_DATA is now
 * handled by char_enum_mailsenders.as with full mail data.
 */
#include "RegionwideCharacterOpcodes.as"
#include "RegionwideCharacterPackets.as"

void main()
{
    Print("[RegionwideCharacter] Initialized (mail handler moved to char_enum_mailsenders.as)");
}
