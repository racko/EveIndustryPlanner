#include "order.h"

std::ostream& operator<<(std::ostream& s, const Order& o) {
    s << "(Order :buy " << int(buy(o).data) << " :issued " << issued(o).data << " :price " << price(o).data
      << " :volumeEntered " << volumeEntered(o).data << " :stationId " << stationId(o).data << " :volume "
      << volume(o).data << " :range " << int(range(o).data) << " :minVolume " << minVolume(o).data << " :duration "
      << duration(o).data << " :type " << type(o).data << " :id " << id(o).data << ')';
    return s;
}
