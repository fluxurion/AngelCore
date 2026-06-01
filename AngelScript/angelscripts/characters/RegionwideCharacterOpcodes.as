/*
 * RegionwideCharacterOpcodes.as
 * Opcodes for regionwide character data (restrictions, mail, etc.)
 * These are CHARACTER opcodes, not BattlePay opcodes
 */

// ============================================================================
// CMSG - Client to Server
// ============================================================================
const uint32 CMSG_GET_REGIONWIDE_CHARACTER_RESTRICTION_AND_MAIL_DATA = 0x400177;  // Client requests restriction and mail data for characters

// ============================================================================
// SMSG - Server to Client
// ============================================================================
const uint32 SMSG_REGIONWIDE_CHARACTER_RESTRICTIONS_DATA            = 0x420019;  // Contains restriction flags per character (CatchUpAvailable, IsRestricted, etc.)
const uint32 SMSG_REGIONWIDE_CHARACTER_MAIL_DATA                    = 0x42001A;  // Server response with mail data for characters
