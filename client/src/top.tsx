import React, { useState } from 'react';
import TvIcon from '@mui/icons-material/Tv';
import { useHistory } from 'react-router-dom';
import './base.css';

export const Top: React.FC = () => {
  const history = useHistory();

  const handleClick = () => {
    history.push('/app/manualviewing');
  };

  return (
    <>
      <div style={{ display: 'flex', flexWrap: 'wrap' }}>
        <div className="common-box-outer">
          <div
            className="common-box-item"
            style={{ height: '150px', width: '150px' }}
            onClick={handleClick}
          >
            <TvIcon style={{ height: '100px', width: 'auto' }} />
            <h4 style={{ textAlign: 'center' }}>Manual Viewing</h4>
          </div>
        </div>
      </div>
    </>
  );
};

export default Top;
