select typeid,name,greatest(0,sum(quantity)) quantity from (
  select invTypes.typeid typeid,invTypes.typeName name,quantity
  from invTypes,invTypeMaterials
  where invTypeMaterials.materialTypeID=invTypes.typeID
  and invTypeMaterials.TypeID=?
  union
  select invTypes.typeid typeid,invTypes.typeName name,
  invTypeMaterials.quantity*r.quantity*-1 quantity
  from invTypes,invTypeMaterials,ramTypeRequirements r,invBlueprintTypes bt
  where invTypeMaterials.materialTypeID=invTypes.typeID
  and invTypeMaterials.TypeID =r.requiredTypeID
  and r.typeID = bt.blueprintTypeID
  and r.activityID = 1 and bt.productTypeID=? and r.recycle=1
) t group by typeid,name;

