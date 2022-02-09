#include <detchannelmaps/TPCChannelMap.hpp>

#include <iostream>
#include <set>
#include <fstream>

int main()
{
  auto pdsp_channel_map = dunedaq::detchannelmaps::make_map("ProtoDUNESP1ChannelMap");
  auto coldbox_channel_map = dunedaq::detchannelmaps::make_map("VDColdboxChannelMap");

  std::set<int> pdsp_collection_indices;
  
  for(int ch=0; ch<256; ++ch){
    auto offline_ch = pdsp_channel_map->get_offline_channel_from_crate_slot_fiber_chan(5, 1, 1, ch);
    int plane = pdsp_channel_map->get_plane_from_offline_channel(offline_ch);
    // It appears that the convention is plane 2 is collection
    // std::cout << ch << " " << offline_ch << " " << (offline_ch%2560) << " " << plane << std::endl;
    if(plane==2){
      pdsp_collection_indices.insert(ch);
    }
  }
  
  std::cout << "There are " << pdsp_collection_indices.size() << " collection channels per frame" << std::endl;

  // Store in a set as a shortcut to sorting
  std::set<int> vd_hitfound_channels;
  
  for(int slot=0; slot < 3; ++slot){
    for(int fiber=1; fiber < 3; ++fiber){
      for(auto ch: pdsp_collection_indices){
        int vd_offline_ch = coldbox_channel_map->get_offline_channel_from_crate_slot_fiber_chan(0, slot, fiber, ch);
        vd_hitfound_channels.insert(vd_offline_ch);
      }
    }
  }

  std::ofstream fout("vd_hitfound_channels.txt");
  
  for(auto ch: vd_hitfound_channels){
     int plane = coldbox_channel_map->get_plane_from_offline_channel(ch);
     fout << ch << "\t" << plane << std::endl;
  }
}
