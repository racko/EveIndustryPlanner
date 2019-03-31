SELECT t.typeName tn, r.quantity qn, r.damagePerJob dmg,t.typeID typeid
FROM ramTypeRequirements r,invTypes t,invBlueprintTypes bt,invGroups g
where r.requiredTypeID = t.typeID and r.typeID = bt.blueprintTypeID
and r.activityID = 1 and bt.productTypeID=? and g.categoryID != 16
and t.groupID = g.groupID;

