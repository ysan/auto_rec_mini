import React, { useState } from 'react';
import { BrowserRouter, Route } from 'react-router-dom';
import ManualViewing from './manualViewing';
import Viewer from './viewer';
import Top from './top';
import './base.css';

const App: React.FC = () => {
  return (
    <>
      <BrowserRouter>
        <Route exact path="/app">
          <Top />
        </Route>

        <Route exact path="/app/manualviewing">
          <ManualViewing />
        </Route>

        <Route exact path="/app/viewer">
          <Viewer />
        </Route>
      </BrowserRouter>
    </>
  );
};

export default App;
