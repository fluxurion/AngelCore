-- AngelDB Warband Groups Tables
-- Migrated from: sql/custom/characters/2026_04_20_00_characters_warband_groups.sql

CREATE TABLE IF NOT EXISTS `warband_groups` (
    `accountId`      int unsigned    NOT NULL,
    `groupId`        bigint unsigned NOT NULL,
    `orderIndex`     tinyint unsigned NOT NULL DEFAULT '0',
    `warbandSceneId` int unsigned    NOT NULL DEFAULT '0',
    `flags`          int unsigned    NOT NULL DEFAULT '0',
    `name`           varchar(255)    NOT NULL,
    PRIMARY KEY (`accountId`, `groupId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `warband_group_members` (
    `accountId`              int unsigned    NOT NULL,
    `groupId`                bigint unsigned NOT NULL,
    `characterGuid`          bigint unsigned NOT NULL,
    `warbandScenePlacementId` int unsigned   NOT NULL DEFAULT '0',
    `type`                   int             NOT NULL DEFAULT '0',
    PRIMARY KEY (`accountId`, `groupId`, `characterGuid`),
    KEY `idx_character` (`characterGuid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
