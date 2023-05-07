import React, { useState, useEffect } from 'react';
import { useHistory } from 'react-router-dom';
import { Utils } from './utils';
import './base.css';
import TvIcon from '@mui/icons-material/Tv';

const _dest = process.env.REACT_APP_DESTINATION;

interface Channel {
  physical_channel: number;
  transport_stream_id: number;
  original_network_id: number;
  remote_control_key_id: number;
  service_ids: number[];
}

interface ChannelName {
  transport_stream_name: string;
  original_network_name: string;
  service_names: string[];
}

//interface SelectedChannel {
//  original_network_id: number;
//  transport_stream_id: number;
//  service_id: number;
//  isSelected: boolean;
//}

export const ManualViewing: React.FC = () => {
  const [channels, setChannels] = useState<Channel[]>([]);
  const [channelNames, setChannelNames] = useState<ChannelName[]>([]);
  //const [selectedChannel, setSelectedChannel] = useState<SelectedChannel>({
  //  original_network_id: 0,
  //  transport_stream_id: 0,
  //  service_id: 0,
  //  isSelected: false
  //});
  const history = useHistory();

  useEffect(() => {
    const f = async () => {
      const channels = await getChannels();
      console.log(JSON.stringify(channels));
      setChannels([...channels]);
    };
    f();
  }, []);

  useEffect(() => {
    const f = async () => {
      const _channelNames: ChannelName[] = [];
      for (const channel of channels) {
        const channelName = await getChannelName(channel);
        _channelNames.push(channelName);
        setChannelNames([..._channelNames]);
      }
    };
    f();
  }, [channels]);

  //useEffect(() => {
  //  window.addEventListener('popstate', overridePopstate, false);
  //  return () =>
  //    window.removeEventListener('popstate', overridePopstate, false);
  //}, [selectedChannel]);

  //const overridePopstate = () => {
  //  console.log(selectedChannel);

  //  if (selectedChannel.isSelected) {
  //    console.log('browzer back');
  //    // isSelected only => false
  //    setSelectedChannel({
  //      original_network_id: selectedChannel.original_network_id,
  //      transport_stream_id: selectedChannel.transport_stream_id,
  //      service_id: selectedChannel.service_id,
  //      isSelected: false
  //    });
  //  } else {
  //    console.log('browzer forward');
  //    // isSelected only => true
  //    setSelectedChannel({
  //      original_network_id: selectedChannel.original_network_id,
  //      transport_stream_id: selectedChannel.transport_stream_id,
  //      service_id: selectedChannel.service_id,
  //      isSelected: true
  //    });
  //  }
  //};

  const getChannels = async (): Promise<Channel[]> => {
    const response = await Utils.request({
      method: 'GET',
      url: _dest + '/api/ctrl/channel/channels',
      timeoutMsec: 5000
    });

    if (response === null || !response.ok) {
      return [];
    }
    const channels: Channel[] = await response.json();
    return channels;
  };

  const getChannelName = async (channel: Channel): Promise<ChannelName> => {
    const channelName: ChannelName = {
      transport_stream_name: '',
      original_network_name: '',
      service_names: []
    };

    const responseNetworkName = await Utils.request({
      method: 'GET',
      url:
        _dest +
        `/api/ctrl/channel/original_network_name/${channel.original_network_id}`,
      timeoutMsec: 5000
    });
    if (responseNetworkName !== null && responseNetworkName.ok) {
      const resNetworkName = await responseNetworkName.json();
      console.log(JSON.stringify(resNetworkName));
      channelName.original_network_name =
        resNetworkName['original_network_name'];
    } else {
      channelName.original_network_name = '-';
    }

    const responseTSName = await Utils.request({
      method: 'GET',
      url:
        _dest +
        `/api/ctrl/channel/transport_stream_name/${channel.original_network_id}/${channel.transport_stream_id}`,
      timeoutMsec: 5000
    });
    if (responseTSName !== null && responseTSName.ok) {
      const resTSName = await responseTSName.json();
      console.log(JSON.stringify(resTSName));
      channelName.transport_stream_name = resTSName['transport_stream_name'];
    } else {
      channelName.transport_stream_name = '-';
    }

    for (const service_id of channel.service_ids) {
      const responseSvcName = await Utils.request({
        method: 'GET',
        url:
          _dest +
          `/api/ctrl/channel/service_name/${channel.original_network_id}/${channel.transport_stream_id}/${service_id}`,
        timeoutMsec: 5000
      });
      if (responseSvcName !== null && responseSvcName.ok) {
        const resSvcName = await responseSvcName.json();
        console.log(service_id + JSON.stringify(resSvcName));
        channelName.service_names.push(resSvcName['service_name']);
      } else {
        channelName.service_names.push('-');
      }
    }

    return channelName;
  };

  const handleClick = (e: React.MouseEvent<HTMLElement>) => {
    const idx: number = parseInt(
      e.currentTarget.getAttribute('data-idx') as string
    );
    const svcidx: number = parseInt(
      e.currentTarget.getAttribute('data-svcidx') as string
    );
    console.log('handleClick', idx, svcidx);
    console.log('orgnid', channels[idx].original_network_id);
    console.log('tsid', channels[idx].transport_stream_id);
    console.log('svcid', channels[idx].service_ids[svcidx]);

    //setSelectedChannel({
    //  original_network_id: channels[idx].original_network_id,
    //  transport_stream_id: channels[idx].transport_stream_id,
    //  service_id: channels[idx].service_ids[svcidx],
    //  isSelected: true
    //});

    history.push({
      pathname: '/app/viewer',
      state: {
        original_network_id: channels[idx].original_network_id,
        transport_stream_id: channels[idx].transport_stream_id,
        service_id: channels[idx].service_ids[svcidx],
        original_network_name: channelNames[idx].original_network_name,
        transport_stream_name: channelNames[idx].transport_stream_name,
        service_name: channelNames[idx].service_names[svcidx]
      }
    });
  };

  return (
    <>
      {channelNames.map((channelName, idx) => (
        <div key={idx} className="common-box-outer">
          <h6>{channelName.original_network_name}</h6>
          <h4>{channelName.transport_stream_name}</h4>
          {channelName.service_names.map((service_name, svcidx) => (
            <div
              className="common-box-item common-box-border"
              key={svcidx}
              data-idx={idx}
              data-svcidx={svcidx}
              onClick={handleClick}
            >
              <div style={{ display: 'flex' }}>
                <TvIcon
                  style={{ width: '30px', height: '30px' }}
                  className="common-box-center-vertical"
                />
                <div className="common-box-center-horizontal">
                  <div>{service_name}</div>
                  <h6>{channels[idx].service_ids[svcidx]}</h6>
                </div>
              </div>
            </div>
          ))}
        </div>
      ))}
    </>
  );
};

export default ManualViewing;
