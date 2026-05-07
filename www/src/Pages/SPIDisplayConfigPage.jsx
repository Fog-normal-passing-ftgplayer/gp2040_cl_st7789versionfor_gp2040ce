import { useEffect, useState } from 'react';
import { Button, Form } from 'react-bootstrap';
import { useTranslation } from 'react-i18next';

import Section from '../Components/Section';
import WebApi from '../Services/WebApi';

export default function SPIDisplayConfigPage() {
    const [loading, setLoading] = useState(true);
    const [saving, setSaving] = useState(false);
    const [saveMessage, setSaveMessage] = useState('');
    const { t } = useTranslation('');

    const [spiDisplayEnabled, setSpiDisplayEnabled] = useState(0);
    const [spiBlock, setSpiBlock] = useState(1);
    const [size, setSize] = useState(100);
    const [spiPinDc, setSpiPinDc] = useState(-1);
    const [spiPinCs, setSpiPinCs] = useState(-1);
    const [spiPinRst, setSpiPinRst] = useState(-1);
    const [spiPinBl, setSpiPinBl] = useState(-1);
    const [spiPinSck, setSpiPinSck] = useState(-1);
    const [spiPinTx, setSpiPinTx] = useState(-1);
    const [spiFlipVertical, setSpiFlipVertical] = useState(0);
    const [spiTextColor, setSpiTextColor] = useState(0xFFFF);
    const [spiBgColor, setSpiBgColor] = useState(0x0000);
    const [spiStatusBarColor, setSpiStatusBarColor] = useState(0xFFFF);
    const [buttonLayout, setButtonLayout] = useState(0);
    const [buttonLayoutRight, setButtonLayoutRight] = useState(0);
    const [layoutDefs, setLayoutDefs] = useState({ buttonLayout: {}, buttonLayoutRight: {} });
    const [spiColOffset, setSpiColOffset] = useState(0);
    const [spiRowOffset, setSpiRowOffset] = useState(0);
    const [flipDisplay, setFlipDisplay] = useState(0);
    const [invertDisplay, setInvertDisplay] = useState(0);
    const [displaySaverTimeout, setDisplaySaverTimeout] = useState(0);
    const [displaySaverMode, setDisplaySaverMode] = useState(0);
    const [inputMode, setInputMode] = useState(1);
    const [turboMode, setTurboMode] = useState(1);
    const [dpadMode, setDpadMode] = useState(1);
    const [socdMode, setSocdMode] = useState(1);
    const [macroMode, setMacroMode] = useState(1);
    const [profileMode, setProfileMode] = useState(0);
    const [inputHistoryEnabled, setInputHistoryEnabled] = useState(0);
    const [turnOffWhenSuspended, setTurnOffWhenSuspended] = useState(0);

    useEffect(() => {
        async function fetchData() {
            try {
                const data = await WebApi.getDisplayOptions();
                const defs = await WebApi.getButtonLayoutDefs();
                setLayoutDefs(defs);
                setSpiDisplayEnabled(data.spiDisplayEnabled ?? 0);
                setSpiBlock(data.spiBlock ?? 1);
                setSize(data.size ?? 100);
                setSpiPinDc(data.spiPinDc ?? -1);
                setSpiPinCs(data.spiPinCs ?? -1);
                setSpiPinRst(data.spiPinRst ?? -1);
                setSpiPinBl(data.spiPinBl ?? -1);
                setSpiPinSck(data.spiPinSck ?? -1);
                setSpiPinTx(data.spiPinTx ?? -1);
                setSpiFlipVertical(data.spiFlipVertical ?? 0);
                setSpiTextColor(data.spiTextColor ?? 0xFFFF);
                setSpiBgColor(data.spiBgColor ?? 0x0000);
                setSpiStatusBarColor(data.spiStatusBarColor ?? 0xFFFF);
                setButtonLayout(data.buttonLayout ?? 0);
                setButtonLayoutRight(data.buttonLayoutRight ?? 0);
                setSpiColOffset(data.spiColOffset ?? 0);
                setSpiRowOffset(data.spiRowOffset ?? 0);
                setFlipDisplay(data.flipDisplay ?? 0);
                setInvertDisplay(data.invertDisplay ?? 0);
                setDisplaySaverTimeout(data.displaySaverTimeout ?? 0);
                setDisplaySaverMode(data.displaySaverMode ?? 0);
                setInputMode(data.inputMode ?? 1);
                setTurboMode(data.turboMode ?? 1);
                setDpadMode(data.dpadMode ?? 1);
                setSocdMode(data.socdMode ?? 1);
                setMacroMode(data.macroMode ?? 1);
                setProfileMode(data.profileMode ?? 0);
                setInputHistoryEnabled(data.inputHistoryEnabled ?? 0);
                setTurnOffWhenSuspended(data.turnOffWhenSuspended ?? 0);
            } catch (e) {
                console.error('Failed to load display options:', e);
            }
            setLoading(false);
        }
        fetchData();
    }, []);

    const handleSave = async (e) => {
        e.preventDefault();
        setSaving(true);
        setSaveMessage('');

        const options = {
            spiDisplayEnabled,
            spiBlock,
            size,
            spiPinDc,
            spiPinCs,
            spiPinRst,
            spiPinBl,
            spiPinSck,
            spiPinTx,
            spiFlipVertical,
            spiTextColor,
            spiBgColor,
            spiStatusBarColor,
            buttonLayout,
            buttonLayoutRight,
            spiColOffset,
            spiRowOffset,
            flipDisplay,
            invertDisplay,
            displaySaverTimeout,
            displaySaverMode,
            inputMode,
            turboMode,
            dpadMode,
            socdMode,
            macroMode,
            profileMode,
            inputHistoryEnabled,
            turnOffWhenSuspended,
            splashMode: 3,          // force splash OFF
            splashDuration: 0,       // force splash OFF
        };

        try {
            const success = await WebApi.setDisplayOptions(options, false);
            if (success) {
                setSaveMessage(t('Common:saved-success-message'));
            } else {
                setSaveMessage(t('Common:saved-error-message'));
            }
        } catch (e) {
            console.error('Save error:', e);
            setSaveMessage(t('Common:saved-error-message'));
        }
        setSaving(false);
    };

    if (loading) return <div>Loading...</div>;

    return (
        <Form onSubmit={handleSave}>
            <Section title={t('SPIDisplayConfig:header-text')}>
                <p className="text-muted">{t('SPIDisplayConfig:sub-header-text')}</p>

                <div className="row mb-3">
                    <label className="col-sm-3 col-form-label">{t('SPIDisplayConfig:spi-display-enabled-label')}</label>
                    <div className="col-sm-9">
                        <Form.Select value={spiDisplayEnabled} onChange={(e) => setSpiDisplayEnabled(parseInt(e.target.value))}>
                            <option value={0}>Disabled</option>
                            <option value={1}>Enabled</option>
                        </Form.Select>
                    </div>
                </div>

                <div className="row mb-3">
                    <label className="col-sm-3 col-form-label">{t('SPIDisplayConfig:spi-block-label')}</label>
                    <div className="col-sm-9">
                        <Form.Select value={spiBlock} onChange={(e) => setSpiBlock(parseInt(e.target.value))}>
                            <option value={0}>SPI 0</option>
                            <option value={1}>SPI 1</option>
                        </Form.Select>
                    </div>
                </div>

                <div className="row mb-3">
                    <label className="col-sm-3 col-form-label">{t('SPIDisplayConfig:size-label')}</label>
                    <div className="col-sm-9">
                        <Form.Select value={size} onChange={(e) => setSize(parseInt(e.target.value))}>
                            <option value={100}>135x240 (Portrait)</option>
                            <option value={101}>240x135 (Landscape)</option>
                        </Form.Select>
                    </div>
                </div>
            </Section>

            <Section title={t('SPIDisplayConfig:pin-header')}>
                <p className="text-muted">{t('SPIDisplayConfig:pin-sub-header')}</p>

                <div className="row mb-3">
                    <label className="col-sm-3 col-form-label">SCK (Clock)</label>
                    <div className="col-sm-9">
                        <Form.Control type="number" min={-1} max={29} value={spiPinSck} onChange={(e) => setSpiPinSck(parseInt(e.target.value) || -1)} />
                    </div>
                </div>

                <div className="row mb-3">
                    <label className="col-sm-3 col-form-label">TX / MOSI (Data)</label>
                    <div className="col-sm-9">
                        <Form.Control type="number" min={-1} max={29} value={spiPinTx} onChange={(e) => setSpiPinTx(parseInt(e.target.value) || -1)} />
                    </div>
                </div>

                <div className="row mb-3">
                    <label className="col-sm-3 col-form-label">{t('SPIDisplayConfig:pin-dc-label')}</label>
                    <div className="col-sm-9">
                        <Form.Control type="number" min={-1} max={29} value={spiPinDc} onChange={(e) => setSpiPinDc(parseInt(e.target.value) || -1)} />
                    </div>
                </div>

                <div className="row mb-3">
                    <label className="col-sm-3 col-form-label">{t('SPIDisplayConfig:pin-cs-label')}</label>
                    <div className="col-sm-9">
                        <Form.Control type="number" min={-1} max={29} value={spiPinCs} onChange={(e) => setSpiPinCs(parseInt(e.target.value) || -1)} />
                    </div>
                </div>

                <div className="row mb-3">
                    <label className="col-sm-3 col-form-label">{t('SPIDisplayConfig:pin-rst-label')}</label>
                    <div className="col-sm-9">
                        <Form.Control type="number" min={-1} max={29} value={spiPinRst} onChange={(e) => setSpiPinRst(parseInt(e.target.value) || -1)} />
                    </div>
                </div>

                <div className="row mb-3">
                    <label className="col-sm-3 col-form-label">{t('SPIDisplayConfig:pin-bl-label')}</label>
                    <div className="col-sm-9">
                        <Form.Control type="number" min={-1} max={29} value={spiPinBl} onChange={(e) => setSpiPinBl(parseInt(e.target.value) || -1)} />
                    </div>
                </div>

                <div className="row mb-3">
                    <label className="col-sm-3 col-form-label">Vertical Flip (上下翻转)</label>
                    <div className="col-sm-9">
                        <Form.Select value={spiFlipVertical} onChange={(e) => setSpiFlipVertical(parseInt(e.target.value))}>
                            <option value={0}>Normal</option>
                            <option value={1}>Flipped</option>
                        </Form.Select>
                    </div>
                </div>

            </Section>

            <Section title="Column/Row Offset (花屏修复)">
                <p className="text-muted">如果屏幕花屏（图像偏移/撕裂），调节行列偏移量。常见值：0,0 或 52,40</p>

                <div className="row mb-3">
                    <label className="col-sm-3 col-form-label">Column Offset (列偏移)</label>
                    <div className="col-sm-9">
                        <Form.Control type="number" min={0} max={320} value={spiColOffset} onChange={(e) => setSpiColOffset(parseInt(e.target.value) || 0)} />
                    </div>
                </div>

                <div className="row mb-3">
                    <label className="col-sm-3 col-form-label">Row Offset (行偏移)</label>
                    <div className="col-sm-9">
                        <Form.Control type="number" min={0} max={320} value={spiRowOffset} onChange={(e) => setSpiRowOffset(parseInt(e.target.value) || 0)} />
                    </div>
                </div>
            </Section>

            <Section title={t('SPIDisplayConfig:display-section-label')}>
                <div className="row mb-3">
                    <label className="col-sm-3 col-form-label">Flip Display</label>
                    <div className="col-sm-9">
                        <Form.Select value={flipDisplay} onChange={(e) => setFlipDisplay(parseInt(e.target.value))}>
                            <option value={0}>None</option>
                            <option value={1}>Flip</option>
                            <option value={2}>Mirror</option>
                            <option value={3}>Flip + Mirror</option>
                        </Form.Select>
                    </div>
                </div>

                <div className="row mb-3">
                    <label className="col-sm-3 col-form-label">Invert Display</label>
                    <div className="col-sm-9">
                        <Form.Select value={invertDisplay} onChange={(e) => setInvertDisplay(parseInt(e.target.value))}>
                            <option value={0}>No</option>
                            <option value={1}>Yes</option>
                        </Form.Select>
                    </div>
                </div>

                <div className="row mb-3">
                    <label className="col-sm-3 col-form-label">Screen Saver</label>
                    <div className="col-sm-9">
                        <Form.Select value={displaySaverMode} onChange={(e) => setDisplaySaverMode(parseInt(e.target.value))}>
                            <option value={0}>Display Off</option>
                            <option value={1}>Snow</option>
                            <option value={2}>Bounce</option>
                            <option value={3}>Pipes</option>
                            <option value={4}>Toast</option>
                            <option value={5}>Nyan Cat</option>
                        </Form.Select>
                    </div>
                </div>

                <div className="row mb-3">
                    <label className="col-sm-3 col-form-label">Saver Timeout (min, 0=off)</label>
                    <div className="col-sm-9">
                        <Form.Control type="number" min={0} value={displaySaverTimeout} onChange={(e) => setDisplaySaverTimeout(parseInt(e.target.value) || 0)} />
                    </div>
                </div>

                <div className="row mb-3">
                    <label className="col-sm-3 col-form-label">Turn Off When Suspended</label>
                    <div className="col-sm-9">
                        <Form.Select value={turnOffWhenSuspended} onChange={(e) => setTurnOffWhenSuspended(parseInt(e.target.value))}>
                            <option value={0}>No</option>
                            <option value={1}>Yes</option>
                        </Form.Select>
                    </div>
                </div>
            </Section>

            <Section title="Status Bar Toggles (状态栏开关)">
                <p className="text-muted">Control what appears on the top status line (same as I2C display)</p>

                <div className="row mb-3">
                    <label className="col-sm-3 col-form-label">Input Mode (XINPUT/PS4...)</label>
                    <div className="col-sm-9">
                        <Form.Select value={inputMode} onChange={(e) => setInputMode(parseInt(e.target.value))}>
                            <option value={0}>Hide</option>
                            <option value={1}>Show</option>
                        </Form.Select>
                    </div>
                </div>

                <div className="row mb-3">
                    <label className="col-sm-3 col-form-label">Turbo Mode (Txx)</label>
                    <div className="col-sm-9">
                        <Form.Select value={turboMode} onChange={(e) => setTurboMode(parseInt(e.target.value))}>
                            <option value={0}>Hide</option>
                            <option value={1}>Show</option>
                        </Form.Select>
                    </div>
                </div>

                <div className="row mb-3">
                    <label className="col-sm-3 col-form-label">D-Pad Mode (D/L/R)</label>
                    <div className="col-sm-9">
                        <Form.Select value={dpadMode} onChange={(e) => setDpadMode(parseInt(e.target.value))}>
                            <option value={0}>Hide</option>
                            <option value={1}>Show</option>
                        </Form.Select>
                    </div>
                </div>

                <div className="row mb-3">
                    <label className="col-sm-3 col-form-label">SOCD Mode (SOCD-N/U/L/F/X)</label>
                    <div className="col-sm-9">
                        <Form.Select value={socdMode} onChange={(e) => setSocdMode(parseInt(e.target.value))}>
                            <option value={0}>Hide</option>
                            <option value={1}>Show</option>
                        </Form.Select>
                    </div>
                </div>

                <div className="row mb-3">
                    <label className="col-sm-3 col-form-label">Macro (M)</label>
                    <div className="col-sm-9">
                        <Form.Select value={macroMode} onChange={(e) => setMacroMode(parseInt(e.target.value))}>
                            <option value={0}>Hide</option>
                            <option value={1}>Show</option>
                        </Form.Select>
                    </div>
                </div>

                <div className="row mb-3">
                    <label className="col-sm-3 col-form-label">Profile #</label>
                    <div className="col-sm-9">
                        <Form.Select value={profileMode} onChange={(e) => setProfileMode(parseInt(e.target.value))}>
                            <option value={0}>Hide</option>
                            <option value={1}>Show</option>
                        </Form.Select>
                    </div>
                </div>
            </Section>

            <Section title="Button Layout (按键布局)">
                <div className="row mb-3">
                    <label className="col-sm-3 col-form-label">Left Layout</label>
                    <div className="col-sm-9">
                        <Form.Select value={buttonLayout} onChange={(e) => setButtonLayout(parseInt(e.target.value))}>
                            {Object.entries(layoutDefs.buttonLayout || {}).map(([name, val]) => (
                                <option key={name} value={val}>{name}</option>
                            ))}
                        </Form.Select>
                    </div>
                </div>
                <div className="row mb-3">
                    <label className="col-sm-3 col-form-label">Right Layout</label>
                    <div className="col-sm-9">
                        <Form.Select value={buttonLayoutRight} onChange={(e) => setButtonLayoutRight(parseInt(e.target.value))}>
                            {Object.entries(layoutDefs.buttonLayoutRight || {}).map(([name, val]) => (
                                <option key={name} value={val}>{name}</option>
                            ))}
                        </Form.Select>
                    </div>
                </div>
            </Section>

            <Section title="Colors (颜色)">
                <p className="text-muted">Pick a color or type RGB565 value. Common: 65535=White, 0=Black, 63488=Red</p>
                <div className="row mb-3">
                    <label className="col-sm-3 col-form-label">Text / Stroke Color</label>
                    <div className="col-sm-9">
                        <div className="input-group">
                            <Form.Control type="color" value={'#' + [((spiTextColor & 0xF800) >> 8), ((spiTextColor & 0x07E0) >> 3), ((spiTextColor & 0x001F) << 3)].map(v => Math.min(255, v | (v >> 5)).toString(16).padStart(2,'0')).join('')} onChange={(e) => { const h=e.target.value; const r=parseInt(h.slice(1,3),16); const g=parseInt(h.slice(3,5),16); const b=parseInt(h.slice(5,7),16); setSpiTextColor(((r>>3)<<11)|((g>>2)<<5)|(b>>3)); }} style={{width: 44, padding: 0, border: 0}} />
                            <Form.Control type="number" value={spiTextColor} onChange={(e) => setSpiTextColor(parseInt(e.target.value) || 0xFFFF)} min={0} max={0xFFFF} className="ms-2" />
                        </div>
                    </div>
                </div>
                <div className="row mb-3">
                    <label className="col-sm-3 col-form-label">Background Color</label>
                    <div className="col-sm-9">
                        <div className="input-group">
                            <Form.Control type="color" value={'#' + [((spiBgColor & 0xF800) >> 8), ((spiBgColor & 0x07E0) >> 3), ((spiBgColor & 0x001F) << 3)].map(v => Math.min(255, v | (v >> 5)).toString(16).padStart(2,'0')).join('')} onChange={(e) => { const h=e.target.value; const r=parseInt(h.slice(1,3),16); const g=parseInt(h.slice(3,5),16); const b=parseInt(h.slice(5,7),16); setSpiBgColor(((r>>3)<<11)|((g>>2)<<5)|(b>>3)); }} style={{width: 44, padding: 0, border: 0}} />
                            <Form.Control type="number" value={spiBgColor} onChange={(e) => setSpiBgColor(parseInt(e.target.value) || 0)} min={0} max={0xFFFF} className="ms-2" />
                        </div>
                    </div>
                </div>
                <div className="row mb-3">
                    <label className="col-sm-3 col-form-label">Status Bar Text Color</label>
                    <div className="col-sm-9">
                        <div className="input-group">
                            <Form.Control type="color" value={'#' + [((spiStatusBarColor & 0xF800) >> 8), ((spiStatusBarColor & 0x07E0) >> 3), ((spiStatusBarColor & 0x001F) << 3)].map(v => Math.min(255, v | (v >> 5)).toString(16).padStart(2,'0')).join('')} onChange={(e) => { const h=e.target.value; const r=parseInt(h.slice(1,3),16); const g=parseInt(h.slice(3,5),16); const b=parseInt(h.slice(5,7),16); setSpiStatusBarColor(((r>>3)<<11)|((g>>2)<<5)|(b>>3)); }} style={{width: 44, padding: 0, border: 0}} />
                            <Form.Control type="number" value={spiStatusBarColor} onChange={(e) => setSpiStatusBarColor(parseInt(e.target.value) || 0xFFFF)} min={0} max={0xFFFF} className="ms-2" />
                        </div>
                    </div>
                </div>
            </Section>

            <div className="mb-3">
                <Button type="submit" disabled={saving} className="me-2">
                    {saving ? 'Saving...' : t('Common:button-save-label')}
                </Button>
                <Button variant="info" onClick={async () => { setSaveMessage('Previewing saver...'); await WebApi.previewSaver(); }} className="me-2">
                    Preview Saver
                </Button>
                <Button variant="outline-danger" onClick={async () => { await WebApi.previewStop(); setSaveMessage('Preview stopped'); }} className="me-2">
                    Stop
                </Button>
                {saveMessage && (
                    <span className={`ms-3 ${saveMessage.includes('Saved') || saveMessage.includes('success') ? 'text-success' : 'text-danger'}`}>
                        {saveMessage}
                    </span>
                )}
            </div>
        </Form>
    );
}
