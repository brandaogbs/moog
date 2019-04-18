function varargout = minimoog_panel(varargin)
% MINIMOOG_PANEL MATLAB code for minimoog_panel.fig
%      MINIMOOG_PANEL, by itself, creates a new MINIMOOG_PANEL or raises the existing
%      singleton*.
%
%      H = MINIMOOG_PANEL returns the handle to a new MINIMOOG_PANEL or the handle to
%      the existing singleton*.
%
%      MINIMOOG_PANEL('CALLBACK',hObject,eventData,handles,...) calls the local
%      function named CALLBACK in MINIMOOG_PANEL.M with the given input arguments.
%
%      MINIMOOG_PANEL('Property','Value',...) creates a new MINIMOOG_PANEL or raises the
%      existing singleton*.  Starting from the left, property value pairs are
%      applied to the GUI before minimoog_panel_OpeningFcn gets called.  An
%      unrecognized property name or invalid value makes property application
%      stop.  All inputs are passed to minimoog_panel_OpeningFcn via varargin.
%
%      *See GUI Options on GUIDE's Tools menu.  Choose "GUI allows only one
%      instance to run (singleton)".
%
% See also: GUIDE, GUIDATA, GUIHANDLES

% Edit the above text to modify the response to help minimoog_panel

% Last Modified by GUIDE v2.5 09-Mar-2017 23:46:33

% Begin initialization code - DO NOT EDIT
gui_Singleton = 1;
gui_State = struct('gui_Name',       mfilename, ...
                   'gui_Singleton',  gui_Singleton, ...
                   'gui_OpeningFcn', @minimoog_panel_OpeningFcn, ...
                   'gui_OutputFcn',  @minimoog_panel_OutputFcn, ...
                   'gui_LayoutFcn',  [] , ...
                   'gui_Callback',   []);
if nargin && ischar(varargin{1})
    gui_State.gui_Callback = str2func(varargin{1});
end

if nargout
    [varargout{1:nargout}] = gui_mainfcn(gui_State, varargin{:});
else
    gui_mainfcn(gui_State, varargin{:});
end
% End initialization code - DO NOT EDIT


% --- Executes just before minimoog_panel is made visible.
function minimoog_panel_OpeningFcn(hObject, eventdata, handles, varargin)
% This function has no output args, see OutputFcn.
% hObject    handle to figure
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
% varargin   command line arguments to minimoog_panel (see VARARGIN)

% Choose default command line output for minimoog_panel
handles.output = hObject;

% Update handles structure
guidata(hObject, handles);

% UIWAIT makes minimoog_panel wait for user response (see UIRESUME)
% uiwait(handles.figure1);


% --- Outputs from this function are returned to the command line.
function varargout = minimoog_panel_OutputFcn(hObject, eventdata, handles) 
% varargout  cell array for returning output args (see VARARGOUT);
% hObject    handle to figure
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Get default command line output from handles structure
varargout{1} = handles.output;


% --- Executes during object creation, after setting all properties.
function osc1_vol_CreateFcn(hObject, eventdata, handles)
% hObject    handle to osc1_vol (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: slider controls usually have a light gray background.
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end


% --- Executes on mouse press over figure background, over a disabled or
% --- inactive control, or over an axes background.
function figure1_WindowButtonUpFcn(hObject, eventdata, handles)
% hObject    handle to figure1 (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
%{
global obj1

if isempty(obj1)
    obj1 = serial('COM2');
else
    fclose(obj1);
    obj1 = obj1(1);
end

fopen(obj1);

drawnow;
osc1_onoff = int2str(get(handles.osc1_onoff, 'Value'));
osc1_vol = int2str(get(handles.osc1_vol, 'Value'));
osc1_tune = int2str(get(handles.osc1_tune, 'Value'));
osc1_range = get(handles.osc1_range, 'SelectedObject');
osc1_wf = get(handles.osc1_wf, 'SelectedObject');

osc2_onoff = int2str(get(handles.osc2_onoff, 'Value'));
osc2_vol = int2str(get(handles.osc2_vol, 'Value'));
osc2_freq = int2str(get(handles.osc2_freq, 'Value'));
osc2_range = get(handles.osc2_range, 'SelectedObject');
osc2_wf = get(handles.osc2_wf, 'SelectedObject');

osc3_onoff = int2str(get(handles.osc3_onoff, 'Value'));
osc3_vol = int2str(get(handles.osc3_vol, 'Value'));
osc3_freq = int2str(get(handles.osc3_freq, 'Value'));
osc3_range = get(handles.osc3_range, 'SelectedObject');
osc3_wf = get(handles.osc3_wf, 'SelectedObject');

noise_onoff = int2str(get(handles.noise_onoff, 'Value'));
noise_vol = int2str(get(handles.noise_vol, 'Value'));
noise_type = get(handles.noise_type, 'SelectedObject');

filter_onoff = int2str(get(handles.filter_onoff, 'Value'));
filter_cutoff = int2str(get(handles.filter_cutoff, 'Value'));
filter_emph = int2str(get(handles.filter_emph, 'Value'));
filter_contour = int2str(get(handles.filter_contour, 'Value'));
filter_attack = int2str(get(handles.filter_attack, 'Value'));
filter_decay = int2str(get(handles.filter_decay, 'Value'));
filter_sustain = int2str(get(handles.filter_sustain, 'Value'));

loud_attack = int2str(get(handles.loud_attack, 'Value'));
loud_decay = int2str(get(handles.loud_decay, 'Value'));
loud_sustain = int2str(get(handles.loud_sustain, 'Value'));

% monta o pacote de dados

pacote = strcat('$',osc1_onoff,'/',osc1_vol,'/',osc1_tune,'/',osc1_range.Tag(2:end),'/',osc1_wf.Tag(2:end),'/',...
    osc2_onoff,'/',osc2_vol,'/',osc2_freq,'/',osc2_range.Tag(2:end),'/',osc2_wf.Tag(2:end),'/',...
    osc3_onoff,'/',osc3_vol,'/',osc3_freq,'/',osc3_range.Tag(2:end),'/',osc3_wf.Tag(2:end),'/',...
    noise_onoff,'/',noise_vol,'/',noise_type.Tag(2:end),'/',...
    loud_attack,'/',loud_decay,'/',loud_sustain,...
    '#');

%filter_onoff,'/',filter_cutoff,'/',filter_emph,'/',filter_contour,'/',filter_attack,'/',filter_decay,'/',filter_sustain,'/',...

fwrite(obj1, pacote);
fscanf(obj1)

%delete(instrfindall);
%}

% --- Executes during object creation, after setting all properties.
function osc2_freq_CreateFcn(hObject, eventdata, handles)
% hObject    handle to osc2_freq (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: slider controls usually have a light gray background.
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end


% --- Executes during object creation, after setting all properties.
function osc2_vol_CreateFcn(hObject, eventdata, handles)
% hObject    handle to osc2_vol (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: slider controls usually have a light gray background.
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end


% --- Executes during object creation, after setting all properties.
function osc3_freq_CreateFcn(hObject, eventdata, handles)
% hObject    handle to osc3_freq (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: slider controls usually have a light gray background.
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end


% --- Executes during object creation, after setting all properties.
function osc3_vol_CreateFcn(hObject, eventdata, handles)
% hObject    handle to osc3_vol (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: slider controls usually have a light gray background.
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end


% --- Executes during object creation, after setting all properties.
function osc1_tune_CreateFcn(hObject, eventdata, handles)
% hObject    handle to osc1_tune (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: slider controls usually have a light gray background.
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end


% --- Executes during object creation, after setting all properties.
function slider10_CreateFcn(hObject, eventdata, handles)
% hObject    handle to slider10 (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: slider controls usually have a light gray background.
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end


% --- Executes during object creation, after setting all properties.
function slider11_CreateFcn(hObject, eventdata, handles)
% hObject    handle to slider11 (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: slider controls usually have a light gray background.
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end

% --- Executes during object creation, after setting all properties.
function slider12_CreateFcn(hObject, eventdata, handles)
% hObject    handle to slider12 (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: slider controls usually have a light gray background.
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end


% --- Executes during object creation, after setting all properties.
function filter_cutoff_CreateFcn(hObject, eventdata, handles)
% hObject    handle to filter_cutoff (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: slider controls usually have a light gray background.
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end


% --- Executes during object creation, after setting all properties.
function filter_emph_CreateFcn(hObject, eventdata, handles)
% hObject    handle to filter_emph (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: slider controls usually have a light gray background.
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end


% --- Executes during object creation, after setting all properties.
function filter_contour_CreateFcn(hObject, eventdata, handles)
% hObject    handle to filter_contour (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: slider controls usually have a light gray background.
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end


% --- Executes during object creation, after setting all properties.
function filter_attack_CreateFcn(hObject, eventdata, handles)
% hObject    handle to filter_attack (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: slider controls usually have a light gray background.
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end


% --- Executes during object creation, after setting all properties.
function filter_decay_CreateFcn(hObject, eventdata, handles)
% hObject    handle to filter_decay (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: slider controls usually have a light gray background.
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end


% --- Executes during object creation, after setting all properties.
function filter_sustain_CreateFcn(hObject, eventdata, handles)
% hObject    handle to filter_sustain (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: slider controls usually have a light gray background.
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end


% --- Executes during object creation, after setting all properties.
function noise_vol_CreateFcn(hObject, eventdata, handles)
% hObject    handle to noise_vol (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: slider controls usually have a light gray background.
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end


% --- Executes during object creation, after setting all properties.
function loud_attack_CreateFcn(hObject, eventdata, handles)
% hObject    handle to loud_attack (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: slider controls usually have a light gray background.
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end


% --- Executes during object creation, after setting all properties.
function loud_decay_CreateFcn(hObject, eventdata, handles)
% hObject    handle to loud_decay (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: slider controls usually have a light gray background.
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end


% --- Executes during object creation, after setting all properties.
function loud_sustain_CreateFcn(hObject, eventdata, handles)
% hObject    handle to loud_sustain (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: slider controls usually have a light gray background.
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end


% --- Executes during object creation, after setting all properties.
function figure1_CreateFcn(hObject, eventdata, handles)
% hObject    handle to figure1 (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

clc;
delete(instrfindall);

global obj1;
obj1 = serial('COM3', 'BaudRate', 115200, 'Terminator', '@', 'Timeout', 5);

disp('Interface iniciada...');


% --- Executes on button press in osc1_onoff.
function osc1_onoff_Callback(hObject, eventdata, handles)
% hObject    handle to osc1_onoff (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hint: get(hObject,'Value') returns toggle state of osc1_onoff

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% OSCILLATOR 1 CALLBACK
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

global obj1

if isempty(obj1)
    obj1 = serial('COM2');
else
    fclose(obj1);
    obj1 = obj1(1);
end

fopen(obj1);

drawnow;
osc1_onoff = get(handles.osc1_onoff, 'Value');
osc1_vol = get(handles.osc1_vol, 'Value');
osc1_tune = get(handles.osc1_tune, 'Value');
osc1_range = get(handles.osc1_range, 'SelectedObject');
osc1_range = str2num(osc1_range.Tag(2:end));
osc1_wf = get(handles.osc1_wf, 'SelectedObject');
osc1_wf = str2num(osc1_wf.Tag(2:end));
init = 255;
address = 1;
endi = 170;

package = uint8([init address osc1_onoff osc1_vol osc1_tune osc1_range osc1_wf endi]);

fwrite(obj1, package);
disp('Comando enviado...');


% --- Executes on button press in osc2_onoff.
function osc2_onoff_Callback(hObject, eventdata, handles)
% hObject    handle to osc2_onoff (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hint: get(hObject,'Value') returns toggle state of osc2_onoff

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% OSCILLATOR 2 CALLBACK
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

global obj1

if isempty(obj1)
    obj1 = serial('COM2');
else
    fclose(obj1);
    obj1 = obj1(1);
end

fopen(obj1);

drawnow;
osc2_onoff = get(handles.osc2_onoff, 'Value');
osc2_vol = get(handles.osc2_vol, 'Value');
osc2_freq = get(handles.osc2_freq, 'Value');
osc2_range = get(handles.osc2_range, 'SelectedObject');
osc2_range = str2num(osc2_range.Tag(2:end));
osc2_wf = get(handles.osc2_wf, 'SelectedObject');
osc2_wf = str2num(osc2_wf.Tag(2:end));
init = 255;
address = 2;
endi = 170;

package = uint8([init address osc2_onoff osc2_vol osc2_freq osc2_range osc2_wf endi]);

fwrite(obj1, package);
disp('Comando enviado...');


% --- Executes on button press in osc3_onoff.
function osc3_onoff_Callback(hObject, eventdata, handles)
% hObject    handle to osc3_onoff (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hint: get(hObject,'Value') returns toggle state of osc3_onoff

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% OSCILLATOR 3 CALLBACK
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

global obj1

if isempty(obj1)
    obj1 = serial('COM2');
else
    fclose(obj1);
    obj1 = obj1(1);
end

fopen(obj1);

drawnow;
osc3_onoff = get(handles.osc3_onoff, 'Value');
osc3_vol = get(handles.osc3_vol, 'Value');
osc3_freq = get(handles.osc3_freq, 'Value');
osc3_range = get(handles.osc3_range, 'SelectedObject');
osc3_range = str2num(osc3_range.Tag(2:end));
osc3_wf = get(handles.osc3_wf, 'SelectedObject');
osc3_wf = str2num(osc3_wf.Tag(2:end));
init = 255;
address = 3;
endi = 170;

package = uint8([init address osc3_onoff osc3_vol osc3_freq osc3_range osc3_wf endi]);

fwrite(obj1, package);
disp('Comando enviado...');


% --- Executes on button press in noise_onoff.
function noise_onoff_Callback(hObject, eventdata, handles)
% hObject    handle to noise_onoff (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hint: get(hObject,'Value') returns toggle state of noise_onoff

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% NOISE CALLBACK
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

global obj1

if isempty(obj1)
    obj1 = serial('COM2');
else
    fclose(obj1);
    obj1 = obj1(1);
end

fopen(obj1);

drawnow;
noise_onoff = get(handles.noise_onoff, 'Value');
noise_vol = get(handles.noise_vol, 'Value');
init = 255;
address = 4;
endi = 170;

package = uint8([init address noise_onoff noise_vol endi]);

fwrite(obj1, package);
disp('Comando enviado...');


% --- Executes on button press in filter_onoff.
function filter_onoff_Callback(hObject, eventdata, handles)
% hObject    handle to filter_onoff (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hint: get(hObject,'Value') returns toggle state of filter_onoff

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% FILTER CALLBACK
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

global obj1

if isempty(obj1)
    obj1 = serial('COM2');
else
    fclose(obj1);
    obj1 = obj1(1);
end

fopen(obj1);

drawnow;
filter_onoff = get(handles.filter_onoff, 'Value');
filter_cutoff = get(handles.filter_cutoff, 'Value');
filter_emph = get(handles.filter_emph, 'Value');
filter_contour = get(handles.filter_contour, 'Value');
filter_attack = get(handles.filter_attack, 'Value');
filter_decay = get(handles.filter_decay, 'Value');
filter_sustain = get(handles.filter_sustain, 'Value');
init = 255;
address = 5;
endi = 170;

package = uint8([init address filter_onoff filter_cutoff filter_emph filter_contour filter_attack filter_decay filter_sustain endi]);

fwrite(obj1, package);
disp('Comando enviado...');


% --- Executes on slider movement.
function loud_attack_Callback(hObject, eventdata, handles)
% hObject    handle to loud_attack (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'Value') returns position of slider
%        get(hObject,'Min') and get(hObject,'Max') to determine range of slider

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% LOUDNESS CONTOUR CALLBACK
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

global obj1

if isempty(obj1)
    obj1 = serial('COM2');
else
    fclose(obj1);
    obj1 = obj1(1);
end

fopen(obj1);

drawnow;
loud_attack = get(handles.loud_attack, 'Value');
loud_decay = get(handles.loud_decay, 'Value');
loud_sustain = get(handles.loud_sustain, 'Value');
init = 255;
address = 6;
endi = 170;

package = uint8([init address loud_attack loud_decay loud_sustain endi]);

fwrite(obj1, package);
disp('Comando enviado...');


% --- Executes on slider movement.
function lfo1_vol_Callback(hObject, eventdata, handles)
% hObject    handle to lfo1_vol (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'Value') returns position of slider
%        get(hObject,'Min') and get(hObject,'Max') to determine range of slider


% --- Executes during object creation, after setting all properties.
function lfo1_vol_CreateFcn(hObject, eventdata, handles)
% hObject    handle to lfo1_vol (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: slider controls usually have a light gray background.
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end


% --- Executes on button press in lfo1_onoff.
function lfo1_onoff_Callback(hObject, eventdata, handles)
% hObject    handle to lfo1_onoff (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hint: get(hObject,'Value') returns toggle state of lfo1_onoff

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% LFO VOLUME CALLBACK
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

global obj1

if isempty(obj1)
    obj1 = serial('COM2');
else
    fclose(obj1);
    obj1 = obj1(1);
end

fopen(obj1);

drawnow;
lfo1_onoff = get(handles.lfo1_onoff, 'Value');
lfo1_vol = get(handles.lfo1_vol, 'Value');
lfo1_freq = get(handles.lfo1_freq, 'Value');
init = 255;
address = 7;
endi = 170;

package = uint8([init address lfo1_onoff lfo1_vol lfo1_freq endi]);

fwrite(obj1, package);
disp('Comando enviado...');


% --- Executes on slider movement.
function lfo1_freq_Callback(hObject, eventdata, handles)
% hObject    handle to lfo1_freq (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'Value') returns position of slider
%        get(hObject,'Min') and get(hObject,'Max') to determine range of slider


% --- Executes during object creation, after setting all properties.
function lfo1_freq_CreateFcn(hObject, eventdata, handles)
% hObject    handle to lfo1_freq (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: slider controls usually have a light gray background.
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end


% --- Executes on slider movement.
function lfo2_vol_Callback(hObject, eventdata, handles)
% hObject    handle to lfo2_vol (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'Value') returns position of slider
%        get(hObject,'Min') and get(hObject,'Max') to determine range of slider


% --- Executes during object creation, after setting all properties.
function lfo2_vol_CreateFcn(hObject, eventdata, handles)
% hObject    handle to lfo2_vol (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: slider controls usually have a light gray background.
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end


% --- Executes on button press in lfo2_onoff.
function lfo2_onoff_Callback(hObject, eventdata, handles)
% hObject    handle to lfo2_onoff (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hint: get(hObject,'Value') returns toggle state of lfo2_onoff

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% LFO FREQUENCY CALLBACK
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

global obj1

if isempty(obj1)
    obj1 = serial('COM2');
else
    fclose(obj1);
    obj1 = obj1(1);
end

fopen(obj1);

drawnow;
lfo2_onoff = get(handles.lfo2_onoff, 'Value');
lfo2_vol = get(handles.lfo2_vol, 'Value');
lfo2_freq = get(handles.lfo2_freq, 'Value');
init = 255;
address = 8;
endi = 170;

package = uint8([init address lfo2_onoff lfo2_vol lfo2_freq endi]);

fwrite(obj1, package);
disp('Comando enviado...');


% --- Executes on slider movement.
function lfo2_freq_Callback(hObject, eventdata, handles)
% hObject    handle to lfo2_freq (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'Value') returns position of slider
%        get(hObject,'Min') and get(hObject,'Max') to determine range of slider


% --- Executes during object creation, after setting all properties.
function lfo2_freq_CreateFcn(hObject, eventdata, handles)
% hObject    handle to lfo2_freq (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: slider controls usually have a light gray background.
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end
