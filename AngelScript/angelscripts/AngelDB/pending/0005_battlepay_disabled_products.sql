-- AngelDB: Disabled Products Table
-- Products listed here are hidden from the in-game shop.

CREATE TABLE IF NOT EXISTS `battlepay_disabled_products` (
  `ProductID` int(10) unsigned NOT NULL,
  PRIMARY KEY (`ProductID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
